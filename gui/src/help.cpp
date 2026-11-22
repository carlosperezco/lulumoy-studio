/**
 * @file help.cpp
 * @brief Implementación de la apertura de la guía matemática.
 */
#include "help.h"

#include <filesystem>

#include "cvcore/image_io.h"

namespace cvgui {

namespace {

namespace fs = std::filesystem;

std::string g_resourceDir;
const I18n* g_i18n = nullptr;

/// Ruta de la página en el primer idioma en que exista; vacío si no existe.
std::string findPage(const std::string& page) {
    if (!g_i18n) return "";
    std::error_code ec;
    for (const std::string& lang : {g_i18n->language(), std::string("en"), std::string("es")}) {
        fs::path path = cvcore::fromUtf8(g_resourceDir) / "help" / cvcore::fromUtf8(lang) /
                        cvcore::fromUtf8(page + ".html");
        if (!fs::is_regular_file(path, ec)) continue;
        // g_filename_to_uri() exige una ruta absoluta.
        const fs::path absolute = fs::absolute(path, ec);
        return cvcore::toUtf8(ec ? path : absolute.lexically_normal());
    }
    return "";
}

} // namespace

void initHelp(const std::string& resourceDir, const I18n& i18n) {
    g_resourceDir = resourceDir;
    g_i18n = &i18n;
}

bool hasHelp(const std::string& page) {
    return !findPage(page).empty();
}

void openHelp(GtkWindow* parent, const std::string& page) {
    std::string path = findPage(page);
    if (path.empty()) path = findPage("index");

    GError* error = nullptr;
    bool opened = false;
    if (!path.empty()) {
        if (gchar* uri = g_filename_to_uri(path.c_str(), nullptr, &error)) {
            opened = gtk_show_uri_on_window(parent, uri, GDK_CURRENT_TIME, &error);
            g_free(uri);
        }
    }
    if (opened) return;

    const std::string text = (g_i18n ? g_i18n->tr("help.not_found") : std::string("Cannot open the help page")) +
                             "\n" + (path.empty() ? g_resourceDir + "/help" : path) +
                             (error ? std::string("\n") + error->message : std::string());
    g_clear_error(&error);
    GtkWidget* dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s",
                                               text.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

} // namespace cvgui
