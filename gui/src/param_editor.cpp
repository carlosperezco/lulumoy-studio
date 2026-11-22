/**
 * @file param_editor.cpp
 * @brief Implementación de la tabla de controles de parámetros.
 */
#include "param_editor.h"

namespace cvgui {

using cvcore::ParamSpec;
using cvcore::ParamType;

ParamEditor::ParamEditor(const I18n& i18n, std::string prefix, std::vector<ParamSpec> specs,
                         const cvcore::ParamValues& initial)
    : specs_(std::move(specs)), initial_(cvcore::sanitize(specs_, initial)) {
    grid_ = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid_), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid_), 12);

    int row = 0;
    for (const ParamSpec& spec : specs_) {
        if (spec.type == ParamType::Point) { // se elige con el ratón, no aquí
            editors_.push_back(nullptr);
            continue;
        }
        const std::string key = prefix + "." + spec.key;
        const double value = cvcore::getReal(initial_, spec.key);

        GtkWidget* label = gtk_label_new(i18n.tr(key, spec.label).c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_grid_attach(GTK_GRID(grid_), label, 0, row, 1, 1);

        GtkWidget* editor = nullptr;
        switch (spec.type) {
            case ParamType::Int:
            case ParamType::Double:
                editor = gtk_spin_button_new_with_range(spec.min, spec.max, spec.step > 0.0 ? spec.step : 1.0);
                gtk_spin_button_set_digits(GTK_SPIN_BUTTON(editor), spec.type == ParamType::Int ? 0 : 2);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(editor), value);
                gtk_entry_set_activates_default(GTK_ENTRY(editor), TRUE);
                g_signal_connect(editor, "value-changed", G_CALLBACK(&ParamEditor::changed), this);
                break;
            case ParamType::Bool:
                editor = gtk_check_button_new();
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editor), value != 0.0);
                g_signal_connect(editor, "toggled", G_CALLBACK(&ParamEditor::changed), this);
                break;
            case ParamType::Choice:
                editor = gtk_combo_box_text_new();
                for (const std::string& choice : spec.choices) {
                    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(editor),
                                                   i18n.tr(key + "." + choice, choice).c_str());
                }
                gtk_combo_box_set_active(GTK_COMBO_BOX(editor), static_cast<int>(value));
                g_signal_connect(editor, "changed", G_CALLBACK(&ParamEditor::changed), this);
                break;
            case ParamType::Point:
                break;
        }
        gtk_widget_set_hexpand(editor, TRUE);
        gtk_grid_attach(GTK_GRID(grid_), editor, 1, row, 1, 1);
        editors_.push_back(editor);
        ++row;
    }
}

bool ParamEditor::hasControls() const {
    for (GtkWidget* editor : editors_) {
        if (editor) return true;
    }
    return false;
}

void ParamEditor::commit() {
    for (std::size_t i = 0; i < specs_.size(); ++i) {
        const ParamType type = specs_[i].type;
        if (editors_[i] && (type == ParamType::Int || type == ParamType::Double)) {
            gtk_spin_button_update(GTK_SPIN_BUTTON(editors_[i]));
        }
    }
}

cvcore::ParamValues ParamEditor::values() const {
    cvcore::ParamValues chosen = initial_; // conserva los puntos elegidos con el ratón
    for (std::size_t i = 0; i < specs_.size(); ++i) {
        GtkWidget* editor = editors_[i];
        if (!editor) continue;
        switch (specs_[i].type) {
            case ParamType::Int:
            case ParamType::Double:
                chosen[specs_[i].key] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(editor));
                break;
            case ParamType::Bool:
                chosen[specs_[i].key] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(editor)) ? 1.0 : 0.0;
                break;
            case ParamType::Choice:
                chosen[specs_[i].key] = gtk_combo_box_get_active(GTK_COMBO_BOX(editor));
                break;
            case ParamType::Point:
                break;
        }
    }
    return cvcore::sanitize(specs_, chosen);
}

void ParamEditor::changed(GtkWidget*, gpointer self) {
    auto* editor = static_cast<ParamEditor*>(self);
    if (editor->onChanged_) editor->onChanged_();
}

} // namespace cvgui
