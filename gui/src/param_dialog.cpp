/**
 * @file param_dialog.cpp
 * @brief Implementación del diálogo genérico de parámetros con vista previa.
 */
#include "param_dialog.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "cvcore/error.h"
#include "help.h"
#include "image_view.h"
#include "param_editor.h"

namespace cvgui {

namespace {

/// Tamaño inicial de la zona de vista previa.
const int kPreviewWidth = 560;
const int kPreviewHeight = 420;
/// Pausa tras un cambio antes de recalcular (ms).
const guint kDelayMs = 250;

/// Opciones de la vista previa que se conservan entre un diálogo y el siguiente.
struct Remembered {
    bool enabled = true;
    int zoom = 0; ///< 0 = ajustar, 1 = 100 %.
};
Remembered remembered;

/// Estado compartido por los controles del diálogo.
struct State {
    const I18n* i18n = nullptr;
    const cvcore::Operation* op = nullptr;
    const cv::Mat* image = nullptr;
    ParamEditor* editor = nullptr;
    ImageView* view = nullptr;
    GtkWidget* enabled = nullptr; ///< Casilla "Vista previa".
    GtkWidget* show = nullptr;    ///< Lista: resultado / original.
    GtkWidget* zoom = nullptr;    ///< Lista: ajustar / 100 %.
    GtkWidget* stats = nullptr;   ///< Tamaño del resultado y tiempo de cálculo.
    GtkWidget* pointer = nullptr; ///< Píxel bajo el ratón.
    guint timer = 0;              ///< Recálculo pendiente (0 = ninguno).
    cv::Mat result;               ///< Último resultado calculado.
    cvcore::ParamValues resultValues; ///< Valores con los que se calculó.
    std::string resultText;
};

std::string format(std::string text, std::initializer_list<std::string> values) {
    int i = 1;
    for (const std::string& value : values) {
        const std::string mark = "%" + std::to_string(i++);
        const std::size_t pos = text.find(mark);
        if (pos != std::string::npos) text.replace(pos, mark.size(), value);
    }
    return text;
}

/// "1920 × 1080 px · 8 bits · color".
std::string describe(const I18n& i18n, const cv::Mat& m) {
    const char* kind = m.channels() == 1 ? "info.gray" : m.channels() == 4 ? "info.color_alpha" : "info.color";
    const std::string sep = " \xC2\xB7 ";
    return std::to_string(m.cols) + " \xC3\x97 " + std::to_string(m.rows) + " px" + sep +
           (m.depth() == CV_16U ? "16 " : "8 ") + i18n.tr("info.bits") + sep + i18n.tr(kind);
}

bool showingOriginal(const State& s) {
    return !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s.enabled)) ||
           gtk_combo_box_get_active(GTK_COMBO_BOX(s.show)) == 1;
}

/// Imagen que se ve ahora en la vista previa.
const cv::Mat& shown(const State& s) {
    return showingOriginal(s) || s.result.empty() ? *s.image : s.result;
}

/// Muestra el resultado o el original con el zoom elegido (no recalcula).
void render(State& s) {
    const cv::Mat& m = shown(s);
    double zoom = 1.0;
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(s.zoom)) == 0) {
        int w = gtk_widget_get_allocated_width(s.view->scrolled()) - 6;
        int h = gtk_widget_get_allocated_height(s.view->scrolled()) - 6;
        if (w < 50 || h < 50) { w = kPreviewWidth; h = kPreviewHeight; } // todavía sin tamaño
        zoom = std::min(static_cast<double>(w) / m.cols, static_cast<double>(h) / m.rows);
        zoom = std::min(zoom, 16.0); // las imágenes pequeñas se amplían: se ven los píxeles
    }
    s.view->setImage(m, zoom);
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s.enabled))) {
        gtk_label_set_text(GTK_LABEL(s.stats), s.i18n->tr("prev.off").c_str());
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(s.show)) == 1) {
        gtk_label_set_text(GTK_LABEL(s.stats), (s.i18n->tr("prev.view.original") + ": " + describe(*s.i18n, *s.image)).c_str());
    } else {
        gtk_label_set_text(GTK_LABEL(s.stats), s.resultText.c_str());
    }
}

/// Aplica la operación con los valores actuales y muestra el resultado.
void recompute(State& s) {
    const I18n& i18n = *s.i18n;
    const cvcore::ParamValues values = s.editor->values();
    try {
        const auto start = std::chrono::steady_clock::now();
        cv::Mat out = s.op->apply(*s.image, values);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (out.empty()) throw cvcore::Error("operation_failed", "The operation failed", s.op->id());
        s.result = out;
        s.resultValues = values;
        s.resultText = format(i18n.tr("prev.result"), {describe(i18n, out), std::to_string(ms)});
    } catch (const cvcore::Error& e) {
        s.result.release();
        s.resultText = i18n.tr("err." + e.code(), e.message());
    } catch (const std::exception& e) {
        s.result.release();
        s.resultText = e.what();
    }
    render(s);
}

gboolean onCompute(gpointer data) {
    auto* s = static_cast<State*>(data);
    s->timer = 0;
    recompute(*s);
    return G_SOURCE_REMOVE;
}

/// Primero muestra "Calculando…" y deja que se dibuje; luego calcula.
gboolean onDelay(gpointer data) {
    auto* s = static_cast<State*>(data);
    gtk_label_set_text(GTK_LABEL(s->stats), s->i18n->tr("vecdlg.computing").c_str());
    s->timer = g_timeout_add(30, onCompute, s);
    return G_SOURCE_REMOVE;
}

void cancelTimer(State& s) {
    if (s.timer != 0) g_source_remove(s.timer);
    s.timer = 0;
}

/// Programa un recálculo; si ya había uno pendiente, lo aplaza.
void schedule(State& s, guint delay) {
    cancelTimer(s);
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s.enabled))) return;
    s.timer = g_timeout_add(delay, onDelay, &s);
}

void onEnabled(GtkToggleButton*, gpointer data) {
    auto* s = static_cast<State*>(data);
    const bool on = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->enabled));
    gtk_widget_set_sensitive(s->show, on);
    if (on && (s->result.empty() || s->resultValues != s->editor->values())) {
        schedule(*s, 0);
    } else {
        cancelTimer(*s);
        render(*s);
    }
}

/**
 * @brief Marca sobre el original la zona que indica la operación (Operation::region).
 *
 * La zona se rodea con una línea y lo que queda fuera se oscurece un poco.
 */
gboolean onCanvasDraw(GtkWidget*, cairo_t* cr, gpointer data) {
    auto* s = static_cast<State*>(data);
    if (!showingOriginal(*s)) return FALSE;
    const std::vector<cv::Point2f> region = s->op->region(s->image->size(), s->editor->values());
    if (region.size() < 2) return FALSE;
    const double zoom = s->view->zoom();
    auto path = [&] {
        for (std::size_t i = 0; i < region.size(); ++i) {
            if (i == 0) cairo_move_to(cr, region[i].x * zoom, region[i].y * zoom);
            else cairo_line_to(cr, region[i].x * zoom, region[i].y * zoom);
        }
        cairo_close_path(cr);
    };
    cairo_new_path(cr);
    cairo_rectangle(cr, 0, 0, s->image->cols * zoom, s->image->rows * zoom); // lo de fuera, más oscuro
    path();
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.45);
    cairo_fill(cr);
    for (int pass = 0; pass < 2; ++pass) { // borde negro y encima amarillo: se ve sobre cualquier fondo
        cairo_new_path(cr);
        path();
        cairo_set_line_width(cr, pass == 0 ? 3.5 : 1.5);
        if (pass == 0) cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        else cairo_set_source_rgb(cr, 1.0, 0.9, 0.0);
        cairo_stroke(cr);
    }
    return FALSE;
}

void onViewChanged(GtkComboBox*, gpointer data) {
    render(*static_cast<State*>(data));
}

GtkWidget* combo(const I18n& i18n, std::initializer_list<const char*> keys, int active) {
    GtkWidget* box = gtk_combo_box_text_new();
    for (const char* key : keys) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(box), i18n.tr(key).c_str());
    gtk_combo_box_set_active(GTK_COMBO_BOX(box), active);
    return box;
}

GtkWidget* label(const std::string& text) {
    GtkWidget* l = gtk_label_new(text.c_str());
    gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
    return l;
}

} // namespace

bool runParamDialog(GtkWindow* parent, const I18n& i18n, const cvcore::Operation& op,
                    cvcore::ParamValues& values, const cv::Mat& image, cv::Mat* result) {
    const std::string prefix = "op." + op.id();
    const std::string title = i18n.tr(prefix, op.label());
    const std::string cancel = i18n.tr("dialog.cancel");
    const std::string accept = i18n.tr("dialog.apply");

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        title.c_str(), parent, static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        cancel.c_str(), GTK_RESPONSE_CANCEL, accept.c_str(), GTK_RESPONSE_OK, nullptr);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    const bool help = hasHelp(op.id());
    if (help) gtk_dialog_add_button(GTK_DIALOG(dialog), i18n.tr("dialog.help").c_str(), GTK_RESPONSE_HELP);

    ParamEditor editor(i18n, prefix, op.params(), values);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    const bool preview = !image.empty();

    State state;
    std::unique_ptr<ImageView> viewer; // solo con vista previa
    if (!preview) {
        gtk_container_add(GTK_CONTAINER(content), editor.widget());
    } else {
        state.i18n = &i18n;
        state.op = &op;
        state.image = &image;
        state.editor = &editor;
        viewer = std::make_unique<ImageView>();
        ImageView& view = *viewer;
        state.view = &view;

        // Columna izquierda: parámetros y opciones de la vista previa.
        GtkWidget* left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_box_pack_start(GTK_BOX(left), editor.widget(), FALSE, FALSE, 0);
        GtkWidget* options = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(options), 8);
        gtk_grid_set_column_spacing(GTK_GRID(options), 12);
        gtk_container_set_border_width(GTK_CONTAINER(options), 12);
        state.enabled = gtk_check_button_new_with_label(i18n.tr("prev.preview").c_str());
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(state.enabled), remembered.enabled);
        state.show = combo(i18n, {"prev.view.result", "prev.view.original"}, 0);
        state.zoom = combo(i18n, {"vecdlg.zoom.fit", "vecdlg.zoom.100"}, remembered.zoom);
        gtk_widget_set_sensitive(state.show, remembered.enabled);
        gtk_grid_attach(GTK_GRID(options), state.enabled, 0, 0, 2, 1);
        gtk_grid_attach(GTK_GRID(options), label(i18n.tr("prev.view")), 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(options), state.show, 1, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(options), label(i18n.tr("vecdlg.zoom")), 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(options), state.zoom, 1, 2, 1, 1);
        gtk_box_pack_start(GTK_BOX(left), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(left), options, FALSE, FALSE, 0);

        // Columna derecha: vista previa con reglas, estado y píxel bajo el ratón.
        GtkWidget* right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(right), 12);
        gtk_widget_set_size_request(view.scrolled(), kPreviewWidth, kPreviewHeight);
        state.stats = label("");
        gtk_label_set_line_wrap(GTK_LABEL(state.stats), TRUE);
        state.pointer = label(" ");
        gtk_box_pack_start(GTK_BOX(right), view.widget(), TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(right), state.stats, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(right), state.pointer, FALSE, FALSE, 0);

        GtkWidget* columns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(columns), left, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(columns), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(columns), right, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(content), columns, TRUE, TRUE, 0);

        editor.setOnChanged([&state] {
            if (showingOriginal(state)) gtk_widget_queue_draw(state.view->canvas()); // la zona marcada cambia ya
            schedule(state, kDelayMs);
        });
        view.setOnPointer([&state](bool inside, double x, double y) {
            const std::string text = inside ? pixelText(shown(state), x, y, state.i18n->tr("info.level")) : " ";
            gtk_label_set_text(GTK_LABEL(state.pointer), text.c_str());
        });
        g_signal_connect_after(view.canvas(), "draw", G_CALLBACK(onCanvasDraw), &state);
        g_signal_connect(state.enabled, "toggled", G_CALLBACK(onEnabled), &state);
        g_signal_connect(state.show, "changed", G_CALLBACK(onViewChanged), &state);
        g_signal_connect(state.zoom, "changed", G_CALLBACK(onViewChanged), &state);
    }

    gtk_widget_show_all(dialog);
    if (preview) {
        render(state);       // original mientras se calcula
        schedule(state, 50); // primer cálculo, con el diálogo ya visible
    }

    gint response;
    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_HELP) {
        openHelp(GTK_WINDOW(dialog), op.id()); // la guía se abre en el navegador; el diálogo sigue abierto
    }
    const bool accepted = response == GTK_RESPONSE_OK;
    if (preview) {
        cancelTimer(state); // que no se ejecute con el diálogo destruido
        remembered.enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state.enabled));
        remembered.zoom = gtk_combo_box_get_active(GTK_COMBO_BOX(state.zoom));
    }
    if (accepted) {
        editor.commit(); // confirma el texto escrito a mano
        values = editor.values();
        if (result && preview && !state.result.empty() && state.resultValues == values) *result = state.result;
    }
    gtk_widget_destroy(dialog);
    return accepted;
}

} // namespace cvgui
