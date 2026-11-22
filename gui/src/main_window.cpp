/**
 * @file main_window.cpp
 * @brief Ventana principal: menús, visor de imagen y pantalla inicial.
 *
 * La ventana no conoce ninguna operación concreta. El menú Edición se
 * genera recorriendo cvcore::Registry, de modo que un módulo nuevo en
 * `core/modules` aparece en el menú sin cambiar este archivo.
 *
 * Si una operación declara parámetros de tipo punto (ParamType::Point), la
 * ventana entra en "modo de selección": pide un clic por punto, dibuja las
 * marcas sobre la imagen y, al completar todos, aplica la operación.
 */
#include "cvgui/gui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtk/gtk.h>

#include <opencv2/core.hpp>

#include "cvcore/error.h"
#include "cvcore/image_info.h"
#include "cvcore/vectorize.h"
#include "cvcore/image_io.h"
#include "cvcore/pipeline.h"
#include "cvcore/registry.h"
#include "help.h"
#include "image_view.h"
#include "lulumoy_version.h"
#include "param_dialog.h"
#include "properties_dialog.h"
#include "vectorize_dialog.h"

namespace cvgui {

namespace {

namespace fs = std::filesystem;

/// Acción asociada a un elemento de menú.
using Action = std::function<void()>;

void onActivate(GtkMenuItem*, gpointer data) {
    (*static_cast<Action*>(data))();
}

void freeAction(gpointer data, GClosure*) {
    delete static_cast<Action*>(data);
}

/// Conecta una acción a la señal "activate" de un elemento de menú.
void connectAction(GtkWidget* item, Action action) {
    g_signal_connect_data(item, "activate", G_CALLBACK(onActivate), new Action(std::move(action)), freeAction,
                          static_cast<GConnectFlags>(0));
}

/// Tipo de archivo de un diálogo de abrir/guardar.
enum class FileKind { Image, Chain, Svg };

/**
 * @brief Ventana principal de la aplicación.
 *
 * Estado que mantiene:
 * - la imagen original (tal como se abrió) y la imagen actual;
 * - la cadena de comandos aplicada desde que se abrió la imagen, que
 *   permite deshacer y guardar la cadena para otras imágenes.
 */
class MainWindow {
public:
    MainWindow(AppConfig& config, I18n& i18n, std::string resourceDir);

    /// Muestra la ventana.
    void show();
    /// Abre una imagen (ruta UTF-8) y la muestra.
    void openFile(const std::string& path);

private:
    // --- construcción de la interfaz ---
    void retranslate();
    void rebuildMenus();
    GtkWidget* ensureMenu(const std::string& path);
    GtkWidget* addItem(GtkWidget* menu, const std::string& label, Action action, guint key = 0,
                       GdkModifierType modifiers = GDK_CONTROL_MASK);
    void addSeparator(GtkWidget* menu);
    void updateWelcome();
    void updateTitle();
    void updateStatus();
    void updateSensitivity();
    void refreshImage();

    // --- acciones de menú ---
    void onOpen();
    void onClose();
    void onSaveAs();
    void onExportSvg();
    void onProperties();
    void onUndo();
    void onRestore();
    void onViewChain();
    void onSaveChain();
    void onApplyChainFile();
    void onAbout();
    void onMission();
    void applyOperation(const cvcore::Operation& op);
    void runOperation(const cvcore::Operation& op, cvcore::ParamValues values);

    // --- selección de puntos con el ratón ---
    bool picking() const { return pick_.op != nullptr; }
    void startPicking(const cvcore::Operation& op, const cvcore::ParamValues& values);
    void stopPicking();
    void removeLastPoint();
    void onImageClick(double x, double y, guint button);
    void drawOverlay(cairo_t* cr);
    void setPickCursor(bool enabled);
    static gboolean onButtonPress(GtkWidget* widget, GdkEvent* event, gpointer self);
    static gboolean onKeyPress(GtkWidget* widget, GdkEvent* event, gpointer self);
    static gboolean onDraw(GtkWidget* widget, cairo_t* cr, gpointer self);
    void requestLanguage(const std::string& code);
    void setLanguage(const std::string& code);
    void setZoom(double zoom);
    double fitZoom() const;

    // --- utilidades ---
    std::string T(const std::string& key) const { return i18n_.tr(key); }
    std::string opLabel(const cvcore::Operation& op) const { return i18n_.tr("op." + op.id(), op.label()); }
    std::string resolve(const std::string& path) const;
    std::string errorText(const std::exception& e) const;
    void showMessage(GtkMessageType type, const std::string& text);
    std::string chooseFile(bool save, FileKind kind, const std::string& title, const std::string& suggested);
    bool hasImage() const { return !current_.empty(); }

    AppConfig& config_;
    I18n& i18n_;
    std::string resourceDir_;

    GtkWidget* window_ = nullptr;
    GtkWidget* vbox_ = nullptr;
    GtkWidget* menubar_ = nullptr;
    GtkWidget* stack_ = nullptr;
    std::unique_ptr<ImageView> view_; ///< Visor con reglas.
    GtkWidget* image_ = nullptr;      ///< Imagen del visor (para dibujar las marcas encima).
    GtkWidget* eventBox_ = nullptr;   ///< Recibe los clics; tiene exactamente el tamaño de la imagen mostrada.
    std::string pointer_;             ///< Posición y valor del píxel bajo el ratón (vacío si está fuera).
    GtkWidget* welcomeLogo_ = nullptr;
    GtkWidget* welcomeLabel_ = nullptr;
    GtkWidget* status_ = nullptr;
    GtkAccelGroup* accel_ = nullptr;

    std::map<std::string, GtkWidget*> menus_; ///< Ruta de menú -> GtkMenu.
    std::vector<GtkWidget*> needImage_;       ///< Elementos activos solo con imagen abierta.
    std::vector<GtkWidget*> needChain_;       ///< Elementos activos solo si hay pasos aplicados.

    cv::Mat original_;        ///< Imagen tal como se abrió.
    cv::Mat current_;         ///< Imagen con la cadena aplicada.
    cvcore::Pipeline chain_;  ///< Pasos aplicados a la imagen actual.
    std::string path_;        ///< Ruta de la imagen abierta.
    std::string lastFolder_;  ///< Última carpeta usada en un diálogo.
    double zoom_ = 1.0;
    std::map<std::string, cvcore::ParamValues> lastParams_; ///< Últimos parámetros usados por operación.
    cvcore::ParamValues vectorParams_;                      ///< Últimos parámetros de vectorización.

    /// Estado del modo de selección de puntos (inactivo si op es nullptr).
    struct Pick {
        const cvcore::Operation* op = nullptr;
        std::vector<cvcore::ParamSpec> points;          ///< Puntos que pide la operación, en orden.
        std::vector<std::pair<double, double>> picked;  ///< Puntos ya marcados, en píxeles de la imagen.
        cvcore::ParamValues values;                     ///< Resto de parámetros de la operación.
    };
    Pick pick_;
};

MainWindow::MainWindow(AppConfig& config, I18n& i18n, std::string resourceDir)
    : config_(config), i18n_(i18n), resourceDir_(std::move(resourceDir)) {
    window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window_), std::max(320, config_.getInt("window_width", 1000)),
                                std::max(240, config_.getInt("window_height", 700)));
    g_signal_connect(window_, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    accel_ = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window_), accel_);
    g_object_unref(accel_); // la ventana conserva su propia referencia

    // Icono configurable.
    const std::string icon = resolve(config_.get("icon", "assets/lulumoy_icon.png"));
    GError* error = nullptr;
    if (!gtk_window_set_icon_from_file(GTK_WINDOW(window_), icon.c_str(), &error)) g_clear_error(&error);

    vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_), vbox_);

    stack_ = gtk_stack_new();

    // Pantalla inicial: logo + mensaje.
    GtkWidget* welcome = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_halign(welcome, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(welcome, GTK_ALIGN_CENTER);
    welcomeLogo_ = gtk_image_new();
    welcomeLabel_ = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(welcomeLabel_), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(welcomeLabel_), TRUE);
    gtk_box_pack_start(GTK_BOX(welcome), welcomeLogo_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(welcome), welcomeLabel_, FALSE, FALSE, 0);
    gtk_stack_add_named(GTK_STACK(stack_), welcome, "welcome");

    // Visor de imagen con reglas y barras de desplazamiento. La posición de
    // un clic, dividida por el zoom, es la coordenada en píxeles de la imagen.
    view_ = std::make_unique<ImageView>();
    image_ = view_->canvas();
    eventBox_ = view_->eventBox();
    g_signal_connect(eventBox_, "button-press-event", G_CALLBACK(&MainWindow::onButtonPress), this);
    g_signal_connect_after(image_, "draw", G_CALLBACK(&MainWindow::onDraw), this);
    g_signal_connect(window_, "key-press-event", G_CALLBACK(&MainWindow::onKeyPress), this);
    view_->setOnPointer([this](bool inside, double x, double y) {
        pointer_ = inside ? pixelText(current_, x, y, T("info.level")) : std::string();
        updateStatus();
    });
    gtk_stack_add_named(GTK_STACK(stack_), view_->widget(), "image");

    gtk_box_pack_start(GTK_BOX(vbox_), stack_, TRUE, TRUE, 0);

    status_ = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(status_), PANGO_ELLIPSIZE_END);
    gtk_widget_set_margin_start(status_, 8);
    gtk_widget_set_margin_end(status_, 8);
    gtk_widget_set_margin_top(status_, 4);
    gtk_widget_set_margin_bottom(status_, 4);
    gtk_box_pack_end(GTK_BOX(vbox_), status_, FALSE, FALSE, 0);

    retranslate();
}

void MainWindow::show() {
    gtk_widget_show_all(window_);
    refreshImage();
}

// ---------------------------------------------------------------------------
// Construcción de la interfaz
// ---------------------------------------------------------------------------

/// Vuelve a crear todos los textos en el idioma activo.
void MainWindow::retranslate() {
    rebuildMenus();
    updateWelcome();
    updateTitle();
    updateStatus();
    updateSensitivity();
}

/// Devuelve el GtkMenu de una ruta ("edit/filters"), creándolo si hace falta.
GtkWidget* MainWindow::ensureMenu(const std::string& path) {
    auto found = menus_.find(path);
    if (found != menus_.end()) return found->second;

    const std::size_t slash = path.rfind('/');
    const std::string leaf = slash == std::string::npos ? path : path.substr(slash + 1);
    GtkWidget* parent = slash == std::string::npos ? menubar_ : ensureMenu(path.substr(0, slash));

    GtkWidget* menu = gtk_menu_new();
    GtkWidget* item = gtk_menu_item_new_with_label(i18n_.tr("menu." + path, leaf).c_str());
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
    menus_[path] = menu;
    return menu;
}

GtkWidget* MainWindow::addItem(GtkWidget* menu, const std::string& label, Action action, guint key,
                               GdkModifierType modifiers) {
    GtkWidget* item = gtk_menu_item_new_with_label(label.c_str());
    connectAction(item, std::move(action));
    if (key != 0) {
        gtk_widget_add_accelerator(item, "activate", accel_, key, modifiers, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    return item;
}

void MainWindow::addSeparator(GtkWidget* menu) {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
}

/// Crea la barra de menús completa. Se llama al arrancar y al cambiar de idioma.
void MainWindow::rebuildMenus() {
    if (menubar_) gtk_widget_destroy(menubar_);
    menus_.clear();
    needImage_.clear();
    needChain_.clear();

    menubar_ = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox_), menubar_, FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(vbox_), menubar_, 0);

    // Archivo
    GtkWidget* file = ensureMenu("file");
    addItem(file, T("file.open"), [this] { onOpen(); }, GDK_KEY_o);
    needImage_.push_back(addItem(file, T("file.save_as"), [this] { onSaveAs(); }, GDK_KEY_s));
    needImage_.push_back(addItem(file, T("file.export_svg"), [this] { onExportSvg(); }, GDK_KEY_e));
    needImage_.push_back(addItem(file, T("file.properties"), [this] { onProperties(); }, GDK_KEY_i));
    needImage_.push_back(addItem(file, T("file.close"), [this] { onClose(); }, GDK_KEY_w));
    addSeparator(file);
    addItem(file, T("file.quit"), [this] { gtk_window_close(GTK_WINDOW(window_)); }, GDK_KEY_q);

    // Edición: acciones fijas + un elemento por cada operación registrada.
    GtkWidget* edit = ensureMenu("edit");
    needChain_.push_back(addItem(edit, T("edit.undo"), [this] { onUndo(); }, GDK_KEY_z));
    needChain_.push_back(addItem(edit, T("edit.restore"), [this] { onRestore(); }));
    addSeparator(edit);
    for (const cvcore::Operation* op : cvcore::Registry::instance().all()) {
        GtkWidget* menu = ensureMenu(op->menu());
        std::string label = opLabel(*op);
        if (!op->params().empty()) label += "\xE2\x80\xA6"; // "…": abre un diálogo
        needImage_.push_back(addItem(menu, label, [this, op] { applyOperation(*op); }));
    }

    // Cadena de comandos
    GtkWidget* chain = ensureMenu("chain");
    needChain_.push_back(addItem(chain, T("chain.view"), [this] { onViewChain(); }));
    needChain_.push_back(addItem(chain, T("chain.save"), [this] { onSaveChain(); }));
    addSeparator(chain);
    needImage_.push_back(addItem(chain, T("chain.apply"), [this] { onApplyChainFile(); }));

    // Ver
    GtkWidget* view = ensureMenu("view");
    needImage_.push_back(addItem(view, T("view.zoom_in"), [this] { setZoom(zoom_ * 1.25); }, GDK_KEY_plus));
    needImage_.push_back(addItem(view, T("view.zoom_out"), [this] { setZoom(zoom_ / 1.25); }, GDK_KEY_minus));
    needImage_.push_back(addItem(view, T("view.zoom_100"), [this] { setZoom(1.0); }, GDK_KEY_1));
    needImage_.push_back(addItem(view, T("view.zoom_fit"), [this] { setZoom(fitZoom()); }, GDK_KEY_0));

    // Idioma: un elemento por cada archivo .lang encontrado.
    GtkWidget* language = ensureMenu("language");
    for (const I18n::Language& lang : i18n_.languages()) {
        GtkWidget* item = gtk_check_menu_item_new_with_label(lang.name.c_str());
        gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
        // Se marca antes de conectar la señal para que no dispare la acción.
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), lang.code == i18n_.language());
        const std::string code = lang.code;
        connectAction(item, [this, code] { requestLanguage(code); });
        gtk_menu_shell_append(GTK_MENU_SHELL(language), item);
    }

    // Ayuda
    GtkWidget* help = ensureMenu("help");
    addItem(help, T("help.guide"), [this] { openHelp(GTK_WINDOW(window_), "index"); }, GDK_KEY_F1,
            static_cast<GdkModifierType>(0));
    addSeparator(help);
    addItem(help, T("help.mission"), [this] { onMission(); });
    addItem(help, T("help.about"), [this] { onAbout(); });

    gtk_widget_show_all(menubar_);
}

/// Actualiza logo y mensaje de la pantalla inicial desde la configuración.
void MainWindow::updateWelcome() {
    const std::string logo = resolve(config_.get("welcome_logo", "assets/logo.png"));
    GError* error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(logo.c_str(), 360, 360, &error);
    if (pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(welcomeLogo_), pixbuf);
        g_object_unref(pixbuf);
    } else {
        g_clear_error(&error);
        gtk_image_clear(GTK_IMAGE(welcomeLogo_));
    }

    // Mensaje: específico del idioma > general > texto por defecto traducido.
    std::string message = config_.get("welcome_message." + i18n_.language());
    if (message.empty()) message = config_.get("welcome_message");
    if (message.empty()) message = T("welcome.default");

    gchar* escaped = g_markup_escape_text(message.c_str(), -1);
    const std::string markup = std::string("<span size=\"x-large\">") + escaped + "</span>";
    g_free(escaped);
    gtk_label_set_markup(GTK_LABEL(welcomeLabel_), markup.c_str());
}

void MainWindow::updateTitle() {
    std::string title = config_.get("window_title", "Lulumoy Studio");
    if (hasImage()) title = cvcore::toUtf8(cvcore::fromUtf8(path_).filename()) + " \xE2\x80\x94 " + title;
    gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
}

void MainWindow::updateStatus() {
    std::string text;
    if (picking() && pick_.picked.size() < pick_.points.size()) {
        // Instrucciones del modo de selección: "Punto 2 de 6: Destino del punto 1 ..."
        const cvcore::ParamSpec& spec = pick_.points[pick_.picked.size()];
        text = T("pick.status");
        const std::string values[3] = {std::to_string(pick_.picked.size() + 1), std::to_string(pick_.points.size()),
                                       i18n_.tr("op." + pick_.op->id() + "." + spec.key, spec.label)};
        for (int i = 0; i < 3; ++i) {
            const std::string mark = "%" + std::to_string(i + 1);
            const std::size_t pos = text.find(mark);
            if (pos != std::string::npos) text.replace(pos, mark.size(), values[i]);
        }
    } else if (!hasImage()) {
        text = T("status.ready");
    } else {
        const char* kind = current_.channels() == 1 ? "info.gray"
                           : current_.channels() == 4 ? "info.color_alpha" : "info.color";
        const std::string sep = " \xC2\xB7 "; // " · "
        text = std::to_string(current_.cols) + " \xC3\x97 " + std::to_string(current_.rows) + " px" + sep +
               (current_.depth() == CV_16U ? "16" : "8") + " " + T("info.bits") + sep + T(kind) + sep +
               T("info.zoom") + " " + std::to_string(static_cast<int>(std::lround(zoom_ * 100.0))) + "%" + sep +
               T("info.steps") + " " + std::to_string(chain_.size());
    }
    if (hasImage() && !pointer_.empty()) text = pointer_ + "    " + text;
    gtk_label_set_text(GTK_LABEL(status_), text.c_str());
}

void MainWindow::updateSensitivity() {
    for (GtkWidget* w : needImage_) gtk_widget_set_sensitive(w, hasImage());
    for (GtkWidget* w : needChain_) gtk_widget_set_sensitive(w, hasImage() && !chain_.empty());
    // Mientras se eligen puntos no se puede usar ningún menú.
    if (menubar_) gtk_widget_set_sensitive(menubar_, !picking());
}

/// Vuelve a dibujar la imagen actual (o la pantalla inicial si no hay imagen).
void MainWindow::refreshImage() {
    pointer_.clear(); // la imagen o el zoom cambiaron: se actualiza al mover el ratón
    if (!hasImage()) {
        view_->clear();
        gtk_stack_set_visible_child_name(GTK_STACK(stack_), "welcome");
    } else {
        view_->setImage(current_, zoom_);
        gtk_stack_set_visible_child_name(GTK_STACK(stack_), "image");
    }
    updateSensitivity();
    updateTitle();
    updateStatus();
}

// ---------------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------------

/// Convierte una ruta de la configuración en absoluta (relativa a la carpeta de recursos).
std::string MainWindow::resolve(const std::string& path) const {
    const fs::path p = cvcore::fromUtf8(path);
    if (p.is_absolute()) return path;
    return cvcore::toUtf8(cvcore::fromUtf8(resourceDir_) / p);
}

/// Texto de un error en el idioma activo.
std::string MainWindow::errorText(const std::exception& e) const {
    if (const auto* error = dynamic_cast<const cvcore::Error*>(&e)) {
        std::string text = i18n_.tr("err." + error->code(), error->message());
        if (!error->detail().empty()) text += "\n" + error->detail();
        return text;
    }
    return e.what();
}

void MainWindow::showMessage(GtkMessageType type, const std::string& text) {
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(window_), GTK_DIALOG_MODAL, type, GTK_BUTTONS_NONE,
                                               "%s", text.c_str());
    gtk_dialog_add_button(GTK_DIALOG(dialog), T("dialog.close").c_str(), GTK_RESPONSE_CLOSE);
    gtk_window_set_title(GTK_WINDOW(dialog), config_.get("window_title", "Lulumoy Studio").c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * @brief Diálogo de abrir o guardar archivo (nativo del sistema).
 * @param suggested Solo al guardar: ruta o nombre propuesto.
 * @return Ruta elegida en UTF-8, o vacío si se canceló.
 */
std::string MainWindow::chooseFile(bool save, FileKind kind, const std::string& title,
                                   const std::string& suggested) {
    GtkFileChooserNative* native = gtk_file_chooser_native_new(
        title.c_str(), GTK_WINDOW(window_), save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        nullptr, nullptr);
    GtkFileChooser* chooser = GTK_FILE_CHOOSER(native);

    GtkFileFilter* filter = gtk_file_filter_new();
    if (kind == FileKind::Image) {
        gtk_file_filter_set_name(filter, T("filter.images").c_str());
        static const char* const kPatterns[] = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tif", "*.tiff", "*.webp",
                                                "*.PNG", "*.JPG", "*.JPEG", "*.BMP", "*.TIF", "*.TIFF", "*.WEBP"};
        for (const char* pattern : kPatterns) gtk_file_filter_add_pattern(filter, pattern);
    } else if (kind == FileKind::Svg) {
        gtk_file_filter_set_name(filter, T("filter.svg").c_str());
        gtk_file_filter_add_pattern(filter, "*.svg");
        gtk_file_filter_add_pattern(filter, "*.SVG");
    } else {
        gtk_file_filter_set_name(filter, T("filter.chains").c_str());
        gtk_file_filter_add_pattern(filter, "*.cvchain");
        if (save) gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    }
    gtk_file_chooser_add_filter(chooser, filter);

    GtkFileFilter* all = gtk_file_filter_new();
    gtk_file_filter_set_name(all, T("filter.all").c_str());
    gtk_file_filter_add_pattern(all, "*");
    gtk_file_chooser_add_filter(chooser, all);

    std::string folder = lastFolder_;
    std::string name = suggested;
    if (save && !suggested.empty()) {
        const fs::path p = cvcore::fromUtf8(suggested);
        if (p.has_parent_path()) folder = cvcore::toUtf8(p.parent_path());
        name = cvcore::toUtf8(p.filename());
    }
    if (!folder.empty()) gtk_file_chooser_set_current_folder(chooser, folder.c_str());
    if (save && !name.empty()) gtk_file_chooser_set_current_name(chooser, name.c_str());

    std::string result;
    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        if (char* filename = gtk_file_chooser_get_filename(chooser)) {
            result = filename;
            g_free(filename);
        }
    }
    g_object_unref(native);

    if (!result.empty()) lastFolder_ = cvcore::toUtf8(cvcore::fromUtf8(result).parent_path());
    return result;
}

// ---------------------------------------------------------------------------
// Acciones
// ---------------------------------------------------------------------------

void MainWindow::openFile(const std::string& path) {
    try {
        cv::Mat image = cvcore::loadImage(path);
        original_ = image;
        current_ = image;
        path_ = path;
        chain_.clear();
        zoom_ = std::min(1.0, fitZoom()); // las imágenes grandes se ajustan a la ventana
        refreshImage();
    } catch (const std::exception& e) {
        showMessage(GTK_MESSAGE_ERROR, errorText(e));
    }
}

void MainWindow::onOpen() {
    const std::string path = chooseFile(false, FileKind::Image, T("dialog.open_image"), "");
    if (!path.empty()) openFile(path);
}

void MainWindow::onClose() {
    original_.release();
    current_.release();
    path_.clear();
    chain_.clear();
    zoom_ = 1.0;
    refreshImage();
}

/// Guardar como: siempre en un archivo nuevo, nunca sobrescribe.
void MainWindow::onSaveAs() {
    if (!hasImage()) return;
    std::string suggested = cvcore::suggestNewPath(path_);
    for (;;) {
        std::string path = chooseFile(true, FileKind::Image, T("dialog.save_image"), suggested);
        if (path.empty()) return;
        if (!cvcore::fromUtf8(path).has_extension()) path += ".png";
        try {
            cvcore::saveImageNew(path, current_);
            gtk_label_set_text(GTK_LABEL(status_), (T("status.saved") + " " + path).c_str());
            return;
        } catch (const std::exception& e) {
            showMessage(GTK_MESSAGE_ERROR, errorText(e));
            suggested = cvcore::suggestNewPath(path); // se vuelve a preguntar con otro nombre
        }
    }
}

/**
 * @brief Exportar SVG: vectoriza la imagen actual en blanco y negro.
 *
 * Primero se ajustan los parámetros con vista previa; después se elige el
 * archivo. Como al guardar imágenes, nunca se sobrescribe un archivo.
 */
void MainWindow::onExportSvg() {
    if (!hasImage()) return;
    if (vectorParams_.empty()) vectorParams_ = cvcore::defaultValues(cvcore::vectorizeParams());
    cvcore::ParamValues values = vectorParams_;
    if (!runVectorizeDialog(GTK_WINDOW(window_), i18n_, current_, values)) return;
    vectorParams_ = values;

    std::string svg;
    try {
        svg = cvcore::toSvg(cvcore::vectorize(current_, values));
    } catch (const std::exception& e) {
        showMessage(GTK_MESSAGE_ERROR, errorText(e));
        return;
    }

    // Nombre propuesto: el de la imagen con extensión .svg (o una variante libre).
    fs::path proposed = cvcore::fromUtf8(path_);
    proposed.replace_extension(".svg");
    std::error_code ec;
    std::string suggested = cvcore::toUtf8(proposed);
    if (fs::exists(proposed, ec)) suggested = cvcore::suggestNewPath(suggested);

    for (;;) {
        std::string path = chooseFile(true, FileKind::Svg, T("dialog.save_svg"), suggested);
        if (path.empty()) return;
        std::string ext = cvcore::toUtf8(cvcore::fromUtf8(path).extension());
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext != ".svg") path += ".svg";
        try {
            cvcore::saveSvgNew(path, svg);
            gtk_label_set_text(GTK_LABEL(status_), (T("status.saved") + " " + path).c_str());
            return;
        } catch (const std::exception& e) {
            showMessage(GTK_MESSAGE_ERROR, errorText(e));
            suggested = cvcore::suggestNewPath(path);
        }
    }
}

/**
 * @brief Propiedades: datos del archivo en disco, metadatos y estado de la
 *        imagen en memoria (que puede diferir del archivo si se ha editado).
 */
void MainWindow::onProperties() {
    if (!hasImage()) return;
    std::vector<cvcore::InfoItem> items = cvcore::fileInfo(path_);
    const std::vector<cvcore::InfoItem> image = cvcore::imageInfo(current_);
    items.insert(items.end(), image.begin(), image.end());
    items.push_back({"image", "image.steps", "Applied steps", std::to_string(chain_.size())});

    const std::string title =
        T("dialog.properties") + " \xE2\x80\x94 " + cvcore::toUtf8(cvcore::fromUtf8(path_).filename());
    runPropertiesDialog(GTK_WINDOW(window_), i18n_, title, items);
}

/**
 * @brief Punto de entrada de una operación elegida en el menú.
 *
 * Si la operación pide puntos, primero se entra en el modo de selección;
 * runOperation() se llama cuando están todos marcados.
 */
void MainWindow::applyOperation(const cvcore::Operation& op) {
    if (!hasImage() || picking()) return;
    const std::vector<cvcore::ParamSpec> specs = op.params();

    cvcore::ParamValues values = cvcore::defaultValues(specs);
    auto last = lastParams_.find(op.id());
    if (last != lastParams_.end()) values = last->second;

    const bool needsPoints = std::any_of(specs.begin(), specs.end(), [](const cvcore::ParamSpec& s) {
        return s.type == cvcore::ParamType::Point;
    });
    if (needsPoints) {
        startPicking(op, values);
    } else {
        runOperation(op, values);
    }
}

/// Pide el resto de parámetros, aplica la operación y la añade a la cadena.
void MainWindow::runOperation(const cvcore::Operation& op, cvcore::ParamValues values) {
    const std::vector<cvcore::ParamSpec> specs = op.params();
    const bool hasDialog = std::any_of(specs.begin(), specs.end(), [](const cvcore::ParamSpec& s) {
        return s.type != cvcore::ParamType::Point;
    });
    cv::Mat result; // la vista previa del diálogo, si se calculó con los valores elegidos
    if (hasDialog && !runParamDialog(GTK_WINDOW(window_), i18n_, op, values, current_, &result)) return;
    values = cvcore::sanitize(specs, values);

    try {
        if (result.empty()) result = op.apply(current_, values);
        if (result.empty()) throw cvcore::Error("operation_failed", "The operation failed", op.id());
        current_ = result;
        chain_.add({op.id(), values});
        lastParams_[op.id()] = values;
        refreshImage();
    } catch (const std::exception& e) {
        showMessage(GTK_MESSAGE_ERROR, errorText(e));
    }
}

// ---------------------------------------------------------------------------
// Selección de puntos con el ratón
// ---------------------------------------------------------------------------

/// Entra en el modo de selección para una operación con parámetros de tipo punto.
void MainWindow::startPicking(const cvcore::Operation& op, const cvcore::ParamValues& values) {
    pick_ = Pick{};
    pick_.op = &op;
    pick_.values = values;
    for (const cvcore::ParamSpec& spec : op.params()) {
        if (spec.type == cvcore::ParamType::Point) pick_.points.push_back(spec);
    }
    setPickCursor(true);
    updateSensitivity();
    updateStatus();
    gtk_widget_queue_draw(image_);
}

/// Sale del modo de selección (al terminar o al cancelar).
void MainWindow::stopPicking() {
    pick_ = Pick{};
    setPickCursor(false);
    updateSensitivity();
    updateStatus();
    gtk_widget_queue_draw(image_);
}

/// Borra el último punto marcado; si no hay ninguno, cancela la operación.
void MainWindow::removeLastPoint() {
    if (pick_.picked.empty()) {
        stopPicking();
        return;
    }
    pick_.picked.pop_back();
    updateStatus();
    gtk_widget_queue_draw(image_);
}

/// Cursor en cruz mientras se eligen puntos.
void MainWindow::setPickCursor(bool enabled) {
    GdkWindow* window = gtk_widget_get_window(eventBox_);
    if (!window) return;
    GdkCursor* cursor = enabled ? gdk_cursor_new_from_name(gtk_widget_get_display(eventBox_), "crosshair") : nullptr;
    gdk_window_set_cursor(window, cursor);
    if (cursor) g_object_unref(cursor);
}

/**
 * @brief Clic sobre la imagen durante el modo de selección.
 * @param x,y    Posición en píxeles de pantalla dentro de la imagen mostrada.
 * @param button 1 = izquierdo (marca un punto), 3 = derecho (borra el último).
 */
void MainWindow::onImageClick(double x, double y, guint button) {
    if (!picking() || !hasImage() || pick_.picked.size() >= pick_.points.size()) return;
    if (button == 3) {
        removeLastPoint();
        return;
    }
    if (button != 1) return;

    // De pantalla a píxeles de la imagen (con decimales si hay zoom de ampliación).
    const double px = std::clamp(std::round(x / zoom_ * 100.0) / 100.0, 0.0, current_.cols - 1.0);
    const double py = std::clamp(std::round(y / zoom_ * 100.0) / 100.0, 0.0, current_.rows - 1.0);
    const cvcore::ParamSpec& spec = pick_.points[pick_.picked.size()];
    pick_.values[spec.key + ".x"] = px;
    pick_.values[spec.key + ".y"] = py;
    pick_.picked.emplace_back(px, py);
    gtk_widget_queue_draw(image_);

    if (pick_.picked.size() < pick_.points.size()) {
        updateStatus();
        return;
    }
    // Todos los puntos marcados: las marcas siguen visibles mientras se
    // muestra el diálogo con el resto de parámetros.
    const cvcore::Operation* op = pick_.op;
    const cvcore::ParamValues values = pick_.values;
    updateStatus();
    runOperation(*op, values);
    stopPicking();
}

/// Dibuja las marcas numeradas y las flechas origen -> destino sobre la imagen.
void MainWindow::drawOverlay(cairo_t* cr) {
    if (!picking()) return;
    const double kPi = 3.14159265358979323846;

    // Flechas: de cada punto enlazado (linkTo) a su destino.
    for (std::size_t i = 0; i < pick_.picked.size(); ++i) {
        const std::string& link = pick_.points[i].linkTo;
        if (link.empty()) continue;
        for (std::size_t j = 0; j < pick_.picked.size(); ++j) {
            if (pick_.points[j].key != link) continue;
            for (int pass = 0; pass < 2; ++pass) { // trazo negro ancho + blanco fino: visible sobre cualquier fondo
                cairo_set_line_width(cr, pass == 0 ? 3.5 : 1.5);
                cairo_set_source_rgb(cr, pass == 0 ? 0.0 : 1.0, pass == 0 ? 0.0 : 1.0, pass == 0 ? 0.0 : 1.0);
                cairo_move_to(cr, pick_.picked[j].first * zoom_, pick_.picked[j].second * zoom_);
                cairo_line_to(cr, pick_.picked[i].first * zoom_, pick_.picked[i].second * zoom_);
                cairo_stroke(cr);
            }
        }
    }

    cairo_set_font_size(cr, 14.0);
    for (std::size_t i = 0; i < pick_.picked.size(); ++i) {
        const double x = pick_.picked[i].first * zoom_;
        const double y = pick_.picked[i].second * zoom_;
        const bool destination = !pick_.points[i].linkTo.empty();

        // Origen en amarillo, destino en cian, ambos con borde negro.
        for (int pass = 0; pass < 2; ++pass) {
            cairo_set_line_width(cr, pass == 0 ? 4.0 : 2.0);
            if (pass == 0) cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            else if (destination) cairo_set_source_rgb(cr, 0.0, 0.9, 1.0);
            else cairo_set_source_rgb(cr, 1.0, 0.9, 0.0);
            cairo_new_path(cr);
            cairo_arc(cr, x, y, 7.0, 0.0, 2.0 * kPi);
            cairo_move_to(cr, x - 11.0, y);
            cairo_line_to(cr, x + 11.0, y);
            cairo_move_to(cr, x, y - 11.0);
            cairo_line_to(cr, x, y + 11.0);
            cairo_stroke(cr);
        }

        // Número del punto, con sombra para que se lea sobre cualquier fondo.
        const std::string number = std::to_string(i + 1);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_move_to(cr, x + 11.0, y - 9.0);
        cairo_show_text(cr, number.c_str());
        if (destination) cairo_set_source_rgb(cr, 0.0, 0.9, 1.0);
        else cairo_set_source_rgb(cr, 1.0, 0.9, 0.0);
        cairo_move_to(cr, x + 10.0, y - 10.0);
        cairo_show_text(cr, number.c_str());
    }
}

gboolean MainWindow::onButtonPress(GtkWidget*, GdkEvent* event, gpointer self) {
    auto* window = static_cast<MainWindow*>(self);
    if (!window->picking()) return FALSE;
    gdouble x = 0.0, y = 0.0;
    guint button = 0;
    if (gdk_event_get_coords(event, &x, &y) && gdk_event_get_button(event, &button)) {
        window->onImageClick(x, y, button);
    }
    return TRUE;
}

/// Teclado en modo de selección: Esc cancela, Retroceso borra el último punto.
gboolean MainWindow::onKeyPress(GtkWidget*, GdkEvent* event, gpointer self) {
    auto* window = static_cast<MainWindow*>(self);
    if (!window->picking()) return FALSE;
    guint key = 0;
    if (!gdk_event_get_keyval(event, &key)) return FALSE;
    if (key == GDK_KEY_Escape) {
        window->stopPicking();
        return TRUE;
    }
    if (key == GDK_KEY_BackSpace) {
        window->removeLastPoint();
        return TRUE;
    }
    return FALSE;
}

gboolean MainWindow::onDraw(GtkWidget*, cairo_t* cr, gpointer self) {
    static_cast<MainWindow*>(self)->drawOverlay(cr);
    return FALSE;
}

/// Deshace el último paso recalculando la cadena desde la imagen original.
void MainWindow::onUndo() {
    if (!hasImage() || chain_.empty()) return;
    cvcore::Pipeline shorter = chain_;
    shorter.removeLast();
    try {
        current_ = shorter.run(original_);
        chain_ = shorter;
        refreshImage();
    } catch (const std::exception& e) {
        showMessage(GTK_MESSAGE_ERROR, errorText(e));
    }
}

void MainWindow::onRestore() {
    if (!hasImage()) return;
    current_ = original_;
    chain_.clear();
    refreshImage();
}

void MainWindow::onViewChain() {
    if (chain_.empty()) {
        showMessage(GTK_MESSAGE_INFO, T("chain.empty"));
        return;
    }
    std::string text = T("chain.title") + "\n";
    int number = 1;
    for (const cvcore::Step& step : chain_.steps()) {
        const cvcore::Operation* op = cvcore::Registry::instance().find(step.opId);
        text += "\n" + std::to_string(number++) + ". " + (op ? opLabel(*op) : step.opId);
        std::string params;
        for (const auto& p : step.params) {
            if (!params.empty()) params += ", ";
            params += p.first + "=" + cvcore::formatNumber(p.second);
        }
        if (!params.empty()) text += " (" + params + ")";
    }
    showMessage(GTK_MESSAGE_INFO, text);
}

void MainWindow::onSaveChain() {
    if (chain_.empty()) {
        showMessage(GTK_MESSAGE_INFO, T("chain.empty"));
        return;
    }
    std::string path = chooseFile(true, FileKind::Chain, T("dialog.save_chain"), "chain.cvchain");
    if (path.empty()) return;
    if (!cvcore::fromUtf8(path).has_extension()) path += ".cvchain";
    try {
        chain_.save(path);
        gtk_label_set_text(GTK_LABEL(status_), (T("status.saved") + " " + path).c_str());
    } catch (const std::exception& e) {
        showMessage(GTK_MESSAGE_ERROR, errorText(e));
    }
}

/// Carga una cadena guardada y la aplica a la imagen actual.
void MainWindow::onApplyChainFile() {
    if (!hasImage()) return;
    const std::string path = chooseFile(false, FileKind::Chain, T("dialog.open_chain"), "");
    if (path.empty()) return;
    try {
        const cvcore::Pipeline loaded = cvcore::Pipeline::load(path);
        if (loaded.empty()) {
            showMessage(GTK_MESSAGE_INFO, T("chain.empty"));
            return;
        }
        current_ = loaded.run(current_);
        for (const cvcore::Step& step : loaded.steps()) chain_.add(step);
        refreshImage();
    } catch (const std::exception& e) {
        showMessage(GTK_MESSAGE_ERROR, errorText(e));
    }
}

/// Diálogo "Acerca de": versión, autor, contacto, web, licencia y librerías.
void MainWindow::onAbout() {
    GtkWidget* dialog = gtk_about_dialog_new();
    GtkAboutDialog* about = GTK_ABOUT_DIALOG(dialog);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window_));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    const std::string name = config_.get("window_title", "Lulumoy Studio");
    const std::string comments = T("about.text") + "\n\n" + T("about.contact") + ": contacto@donatellatech.com\n\nGTK " +
                                 std::to_string(gtk_get_major_version()) + "." +
                                 std::to_string(gtk_get_minor_version()) + "." +
                                 std::to_string(gtk_get_micro_version()) + " \xC2\xB7 OpenCV " + CV_VERSION;
    gtk_about_dialog_set_program_name(about, name.c_str());
    gtk_about_dialog_set_version(about, LULUMOY_VERSION);
    gtk_about_dialog_set_comments(about, comments.c_str());
    gtk_about_dialog_set_copyright(about, "\xC2\xA9 2026 Carlos N. Perez-Montenegro \xC2\xB7 DonatellaTech");
    gtk_about_dialog_set_license_type(about, GTK_LICENSE_MIT_X11);
    gtk_about_dialog_set_website(about, "https://donatellatech.com/");
    gtk_about_dialog_set_website_label(about, "donatellatech.com");

    const gchar* authors[] = {"Carlos N. Perez-Montenegro", "DonatellaTech <contacto@donatellatech.com>", nullptr};
    gtk_about_dialog_set_authors(about, authors);
    const gchar* libraries[] = {"OpenCV https://opencv.org", "GTK https://www.gtk.org", nullptr};
    gtk_about_dialog_add_credit_section(about, T("about.libraries").c_str(), libraries);

    GError* error = nullptr;
    const std::string logo = resolve(config_.get("welcome_logo", "assets/logo.png"));
    if (GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(logo.c_str(), 128, 128, &error)) {
        gtk_about_dialog_set_logo(about, pixbuf);
        g_object_unref(pixbuf);
    } else {
        g_clear_error(&error);
    }

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/// Misión y visión de DonatellaTech, con enlace a la web.
void MainWindow::onMission() {
    const std::string close = T("dialog.close");
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        T("mission.title").c_str(), GTK_WINDOW(window_),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), close.c_str(),
        GTK_RESPONSE_CLOSE, nullptr);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 18);

    GError* error = nullptr;
    const std::string logo = resolve(config_.get("welcome_logo", "assets/logo.png"));
    if (GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(logo.c_str(), 96, 96, &error)) {
        gtk_box_pack_start(GTK_BOX(box), gtk_image_new_from_pixbuf(pixbuf), FALSE, FALSE, 0);
        g_object_unref(pixbuf);
    } else {
        g_clear_error(&error);
    }

    auto section = [&](const std::string& title, const std::string& text) {
        gchar* escaped = g_markup_escape_text(title.c_str(), -1);
        const std::string markup = std::string("<span size=\"large\" weight=\"bold\">") + escaped + "</span>";
        g_free(escaped);
        GtkWidget* heading = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(heading), markup.c_str());
        gtk_label_set_xalign(GTK_LABEL(heading), 0.0f);
        GtkWidget* body = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(body), 0.0f);
        gtk_label_set_line_wrap(GTK_LABEL(body), TRUE);
        // Ancho fijo en caracteres: si no, GTK calcula la altura de las líneas
        // ajustadas para un ancho mínimo y la ventana sale demasiado alta.
        gtk_label_set_width_chars(GTK_LABEL(body), 64);
        gtk_label_set_max_width_chars(GTK_LABEL(body), 64);
        gtk_label_set_selectable(GTK_LABEL(body), TRUE);
        gtk_widget_set_can_focus(body, FALSE);
        gtk_box_pack_start(GTK_BOX(box), heading, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), body, FALSE, FALSE, 0);
    };
    section(T("mission.mission"), T("mission.mission_text"));
    section(T("mission.vision"), T("mission.vision_text"));

    GtkWidget* link = gtk_link_button_new_with_label("https://donatellatech.com/", "donatellatech.com");
    gtk_widget_set_halign(link, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), link, FALSE, FALSE, 0);
    GtkWidget* mail = gtk_link_button_new_with_label("mailto:contacto@donatellatech.com", "contacto@donatellatech.com");
    gtk_widget_set_halign(mail, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), mail, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/// Zoom que hace caber la imagen completa en la ventana.
double MainWindow::fitZoom() const {
    if (!hasImage()) return 1.0;
    const int w = gtk_widget_get_allocated_width(stack_) - 30; // reglas (22 px) y margen
    const int h = gtk_widget_get_allocated_height(stack_) - 30;
    if (w <= 1 || h <= 1) return 1.0;
    return std::min(static_cast<double>(w) / current_.cols, static_cast<double>(h) / current_.rows);
}

void MainWindow::setZoom(double zoom) {
    if (!hasImage()) return;
    // Límite para no crear imágenes de pantalla enormes.
    const double largest = std::max(current_.cols, current_.rows);
    zoom_ = std::clamp(zoom, 0.02, std::max(0.02, std::min(16.0, 16000.0 / largest)));
    refreshImage();
}

/// Datos de un cambio de idioma pendiente.
struct LanguageRequest {
    MainWindow* window;
    std::string code;
    void (MainWindow::*apply)(const std::string&);
};

/**
 * @brief Pide un cambio de idioma.
 *
 * El cambio reconstruye la barra de menús, así que se aplaza hasta que
 * termine de procesarse el clic en el propio menú.
 */
void MainWindow::requestLanguage(const std::string& code) {
    auto* request = new LanguageRequest{this, code, &MainWindow::setLanguage};
    g_idle_add(
        [](gpointer data) -> gboolean {
            auto* r = static_cast<LanguageRequest*>(data);
            (r->window->*(r->apply))(r->code);
            delete r;
            return G_SOURCE_REMOVE;
        },
        request);
}

void MainWindow::setLanguage(const std::string& code) {
    if (i18n_.setLanguage(code)) {
        config_.set("language", code);
        config_.save(); // se recuerda para el próximo arranque
    }
    retranslate();
}

/// Datos para abrir la imagen inicial cuando la ventana ya tiene tamaño.
struct InitialOpen {
    MainWindow* window;
    std::string path;
};

} // namespace

int runGui(int argc, char** argv, AppConfig& config, I18n& i18n, const std::string& resourceDir,
           const std::string& initialImage) {
    gtk_init(&argc, &argv);
    initHelp(resourceDir, i18n);

    MainWindow window(config, i18n, resourceDir);
    window.show();

    if (!initialImage.empty()) {
        g_idle_add(
            [](gpointer data) -> gboolean {
                auto* open = static_cast<InitialOpen*>(data);
                open->window->openFile(open->path);
                delete open;
                return G_SOURCE_REMOVE;
            },
            new InitialOpen{&window, initialImage});
    }

    gtk_main();
    return 0;
}

} // namespace cvgui
