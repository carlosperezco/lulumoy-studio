/**
 * @file vectorize_dialog.cpp
 * @brief Implementación del diálogo de vectorización con vista previa.
 */
#include "vectorize_dialog.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "cvcore/error.h"
#include "cvcore/image_io.h"
#include "cvcore/vectorize.h"
#include "help.h"
#include "param_editor.h"

namespace cvgui {

namespace {

/// Tamaño del área de vista previa en modo "Ajustar".
const int kPreviewWidth = 640;
const int kPreviewHeight = 480;
/// Pausa tras un cambio antes de recalcular (ms).
const guint kDelayMs = 300;

/// Estado compartido por los controles del diálogo.
struct State {
    const I18n* i18n = nullptr;
    const cv::Mat* image = nullptr;
    ParamEditor* editor = nullptr;
    GtkWidget* preview = nullptr; ///< GtkImage.
    GtkWidget* stats = nullptr;   ///< GtkLabel.
    GtkWidget* mode = nullptr;    ///< Lista: resultado / superpuesto / binarizada.
    GtkWidget* zoom = nullptr;    ///< Lista: ajustar / 100 %.
    guint timer = 0;              ///< Recalculo pendiente (0 = ninguno).
    bool valid = false;           ///< Hay un resultado calculado.
    cvcore::VectorResult result;
    cv::Mat gray;                 ///< Original en grises de 8 bits (para superponer).
};

/// Sustituye %1, %2... por los valores dados.
std::string format(std::string text, const std::vector<std::string>& values) {
    for (std::size_t i = 0; i < values.size(); ++i) {
        const std::string mark = "%" + std::to_string(i + 1);
        const std::size_t pos = text.find(mark);
        if (pos != std::string::npos) text.replace(pos, mark.size(), values[i]);
    }
    return text;
}

std::string sizeText(std::size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string((bytes + 512) / 1024) + " kB";
    const double mb = bytes / (1024.0 * 1024.0);
    return std::to_string(static_cast<int>(mb)) + "." + std::to_string(static_cast<int>(mb * 10) % 10) + " MB";
}

/// Copia una imagen RGB de 8 bits a un GdkPixbuf nuevo.
GdkPixbuf* toPixbuf(const cv::Mat& rgb) {
    GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, rgb.cols, rgb.rows);
    if (!pixbuf) return nullptr;
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    const int stride = gdk_pixbuf_get_rowstride(pixbuf);
    for (int y = 0; y < rgb.rows; ++y) {
        std::memcpy(pixels + static_cast<std::size_t>(y) * stride, rgb.ptr(y), static_cast<std::size_t>(rgb.cols) * 3);
    }
    return pixbuf;
}

/// Escala de la vista previa respecto a la imagen original.
double previewScale(const State& s) {
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(s.zoom)) == 1) return 1.0;
    const double fit = std::min(static_cast<double>(kPreviewWidth) / s.image->cols,
                                static_cast<double>(kPreviewHeight) / s.image->rows);
    return std::min(fit, 8.0); // las imágenes pequeñas se amplían: ahí se aprecia el vector
}

/// Dibuja la vista previa con el último resultado (no recalcula).
void render(State& s) {
    if (!s.valid) return;
    const double scale = previewScale(s);
    const int w = std::max(1, static_cast<int>(std::lround(s.image->cols * scale)));
    const int h = std::max(1, static_cast<int>(std::lround(s.image->rows * scale)));
    const int mode = gtk_combo_box_get_active(GTK_COMBO_BOX(s.mode));

    cv::Mat rgb(h, w, CV_8UC3);
    if (mode == 2) { // binarizada: los píxeles que se van a trazar, en negro
        cv::Mat binary;
        // La binarizada está a la resolución de trabajo (×2, ×4): al reducir se promedia.
        cv::resize(s.result.binary, binary, cv::Size(w, h), 0, 0, s.result.binary.cols > w ? cv::INTER_AREA : cv::INTER_NEAREST);
        for (int y = 0; y < h; ++y) {
            const uchar* b = binary.ptr<uchar>(y);
            uchar* out = rgb.ptr<uchar>(y);
            for (int x = 0; x < w; ++x) out[3 * x] = out[3 * x + 1] = out[3 * x + 2] = static_cast<uchar>(255 - b[x]);
        }
    } else {
        const cv::Mat mask = cvcore::renderMask(s.result, scale);
        cv::Mat base;
        if (mode == 1) {
            cv::resize(s.gray, base, cv::Size(w, h), 0, 0, scale < 1.0 ? cv::INTER_AREA : cv::INTER_LINEAR);
        }
        for (int y = 0; y < h; ++y) {
            const uchar* m = mask.ptr<uchar>(y);
            uchar* out = rgb.ptr<uchar>(y);
            if (mode == 0) { // resultado: negro sobre blanco
                for (int x = 0; x < w; ++x) out[3 * x] = out[3 * x + 1] = out[3 * x + 2] = static_cast<uchar>(255 - m[x]);
            } else {         // superpuesto: relleno rojo semitransparente sobre el original aclarado
                const uchar* g = base.ptr<uchar>(y);
                for (int x = 0; x < w; ++x) {
                    const double a = 0.65 * m[x] / 255.0;
                    const double light = 110.0 + g[x] * 0.57;
                    out[3 * x] = static_cast<uchar>(light * (1.0 - a) + 220.0 * a);
                    out[3 * x + 1] = static_cast<uchar>(light * (1.0 - a) + 30.0 * a);
                    out[3 * x + 2] = static_cast<uchar>(light * (1.0 - a) + 30.0 * a);
                }
            }
        }
    }

    if (GdkPixbuf* pixbuf = toPixbuf(rgb)) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(s.preview), pixbuf);
        g_object_unref(pixbuf);
    }
}

/// Recalcula la vectorización con los valores actuales y redibuja.
void recompute(State& s) {
    const I18n& i18n = *s.i18n;
    try {
        s.result = cvcore::vectorize(*s.image, s.editor->values());
        s.valid = true;
        const std::string svg = cvcore::toSvg(s.result);
        std::string text = format(i18n.tr("vecdlg.stats"), {std::to_string(s.result.paths.size()),
                                                            std::to_string(s.result.nodes()), sizeText(svg.size())});
        if (s.result.workScale > 1) text += "  \xC2\xB7  " + format(i18n.tr("vecdlg.work"), {"x" + std::to_string(s.result.workScale)});
        gtk_label_set_text(GTK_LABEL(s.stats), text.c_str());
        render(s);
    } catch (const cvcore::Error& e) {
        gtk_label_set_text(GTK_LABEL(s.stats), i18n.tr("err." + e.code(), e.message()).c_str());
    } catch (const std::exception& e) {
        gtk_label_set_text(GTK_LABEL(s.stats), e.what());
    }
}

gboolean onTimer(gpointer data) {
    auto* s = static_cast<State*>(data);
    s->timer = 0;
    recompute(*s);
    return G_SOURCE_REMOVE;
}

/// Programa un recálculo; si ya había uno pendiente, lo aplaza.
void schedule(State& s, guint delay) {
    if (s.timer != 0) g_source_remove(s.timer);
    s.timer = g_timeout_add(delay, onTimer, &s);
}

void onViewChanged(GtkComboBox*, gpointer data) {
    render(*static_cast<State*>(data));
}

GtkWidget* combo(const I18n& i18n, std::initializer_list<const char*> keys) {
    GtkWidget* box = gtk_combo_box_text_new();
    for (const char* key : keys) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(box), i18n.tr(key).c_str());
    gtk_combo_box_set_active(GTK_COMBO_BOX(box), 0);
    return box;
}

GtkWidget* label(const std::string& text) {
    GtkWidget* l = gtk_label_new(text.c_str());
    gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
    return l;
}

} // namespace

bool runVectorizeDialog(GtkWindow* parent, const I18n& i18n, const cv::Mat& image, cvcore::ParamValues& values) {
    const std::string title = i18n.tr("vecdlg.title");
    const std::string cancel = i18n.tr("dialog.cancel");
    const std::string save = i18n.tr("vecdlg.save");

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        title.c_str(), parent, static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        cancel.c_str(), GTK_RESPONSE_CANCEL, save.c_str(), GTK_RESPONSE_OK, nullptr);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    if (hasHelp("vectorize")) {
        gtk_dialog_add_button(GTK_DIALOG(dialog), i18n.tr("dialog.help").c_str(), GTK_RESPONSE_HELP);
    }

    ParamEditor editor(i18n, "vec", cvcore::vectorizeParams(), values);
    State state;
    state.i18n = &i18n;
    state.image = &image;
    state.editor = &editor;
    cv::Mat gray = cvcore::toGray(image);
    if (gray.depth() != CV_8U) gray.convertTo(state.gray, CV_8U, 1.0 / 257.0);
    else state.gray = gray;

    // Columna izquierda: parámetros.
    GtkWidget* left = editor.widget();

    // Columna derecha: vista previa, sus opciones y estadísticas.
    GtkWidget* right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scrolled), kPreviewWidth + 4);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), kPreviewHeight + 4);
    state.preview = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(scrolled), state.preview);

    GtkWidget* view = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    state.mode = combo(i18n, {"vecdlg.mode.result", "vecdlg.mode.overlay", "vecdlg.mode.binary"});
    state.zoom = combo(i18n, {"vecdlg.zoom.fit", "vecdlg.zoom.100"});
    gtk_box_pack_start(GTK_BOX(view), label(i18n.tr("vecdlg.mode")), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(view), state.mode, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(view), label(i18n.tr("vecdlg.zoom")), FALSE, FALSE, 12);
    gtk_box_pack_start(GTK_BOX(view), state.zoom, FALSE, FALSE, 0);

    state.stats = label(i18n.tr("vecdlg.computing"));
    gtk_label_set_line_wrap(GTK_LABEL(state.stats), TRUE);
    gtk_box_pack_start(GTK_BOX(right), scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right), view, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right), state.stats, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(right), 12);

    GtkWidget* columns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(columns), left, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(columns), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(columns), right, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), columns, TRUE, TRUE, 0);

    editor.setOnChanged([&state] { schedule(state, kDelayMs); });
    g_signal_connect(state.mode, "changed", G_CALLBACK(onViewChanged), &state);
    g_signal_connect(state.zoom, "changed", G_CALLBACK(onViewChanged), &state);

    gtk_widget_show_all(dialog);
    schedule(state, 50); // primer cálculo, con el diálogo ya visible

    gint response;
    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_HELP) {
        openHelp(GTK_WINDOW(dialog), "vectorize");
    }
    const bool accepted = response == GTK_RESPONSE_OK;
    if (state.timer != 0) { // que no se ejecute con el diálogo destruido
        g_source_remove(state.timer);
        state.timer = 0;
    }
    if (accepted) {
        editor.commit();
        values = editor.values();
    }
    gtk_widget_destroy(dialog);
    return accepted;
}

} // namespace cvgui
