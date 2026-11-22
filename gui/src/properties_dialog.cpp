/**
 * @file properties_dialog.cpp
 * @brief Implementación del diálogo "Propiedades".
 */
#include "properties_dialog.h"

namespace cvgui {

void runPropertiesDialog(GtkWindow* parent, const I18n& i18n, const std::string& title,
                         const std::vector<cvcore::InfoItem>& items) {
    const int kCopy = 1; // respuesta propia del botón "Copiar todo"
    const std::string copy = i18n.tr("dialog.copy");
    const std::string close = i18n.tr("dialog.close");

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        title.c_str(), parent, static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        copy.c_str(), kCopy, close.c_str(), GTK_RESPONSE_CLOSE, nullptr);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 600);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    std::string plain; // la misma tabla como texto, para el portapapeles
    std::string group;
    int row = 0;
    for (const cvcore::InfoItem& item : items) {
        if (item.group != group) {
            group = item.group;
            const std::string name = i18n.tr("prop.group." + group, group);
            gchar* escaped = g_markup_escape_text(name.c_str(), -1);
            const std::string markup = std::string("<b>") + escaped + "</b>";
            g_free(escaped);

            GtkWidget* header = gtk_label_new(nullptr);
            gtk_label_set_markup(GTK_LABEL(header), markup.c_str());
            gtk_label_set_xalign(GTK_LABEL(header), 0.0f);
            if (row > 0) gtk_widget_set_margin_top(header, 12);
            gtk_grid_attach(GTK_GRID(grid), header, 0, row++, 2, 1);

            if (!plain.empty()) plain += "\n";
            plain += name + "\n";
        }

        const std::string name = i18n.tr("prop." + item.key, item.label);
        GtkWidget* label = gtk_label_new(name.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_valign(label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(label, 12);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

        GtkWidget* value = gtk_label_new(item.value.c_str());
        gtk_label_set_xalign(GTK_LABEL(value), 0.0f);
        gtk_label_set_selectable(GTK_LABEL(value), TRUE);
        // Seleccionable con el ratón, pero sin recibir el foco del teclado: si no,
        // GTK selecciona un valor al abrir y desplaza la tabla hasta él.
        gtk_widget_set_can_focus(value, FALSE);
        gtk_label_set_line_wrap(GTK_LABEL(value), TRUE);
        gtk_label_set_line_wrap_mode(GTK_LABEL(value), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_max_width_chars(GTK_LABEL(value), 60);
        gtk_widget_set_hexpand(value, TRUE);
        gtk_grid_attach(GTK_GRID(grid), value, 1, row, 1, 1);
        ++row;

        plain += "  " + name + ": " + item.value + "\n";
    }

    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scrolled), grid);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolled, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);
    while (gtk_dialog_run(GTK_DIALOG(dialog)) == kCopy) {
        GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, plain.c_str(), -1);
        gtk_clipboard_store(clipboard); // sigue disponible aunque se cierre la aplicación
    }
    gtk_widget_destroy(dialog);
}

} // namespace cvgui
