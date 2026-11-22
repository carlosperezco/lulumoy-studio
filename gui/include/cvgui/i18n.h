/**
 * @file i18n.h
 * @brief Traducciones de la interfaz (multilenguaje).
 */
#pragma once

#include <map>
#include <string>
#include <vector>

namespace cvgui {

/**
 * @brief Tabla de textos por idioma.
 *
 * Cada idioma es un archivo `<código>.lang` (p. ej. `es.lang`) con líneas
 * `clave = texto` en UTF-8. La clave `lang.name` es el nombre que aparece
 * en el menú de idiomas. Para añadir un idioma basta con copiar un archivo
 * a la carpeta `lang` y traducirlo.
 */
class I18n {
public:
    /// Idioma disponible.
    struct Language {
        std::string code; ///< Código: nombre del archivo sin extensión ("es", "en", "it").
        std::string name; ///< Nombre visible ("Español").
    };

    /// Carga todos los archivos `*.lang` de una carpeta (ruta UTF-8).
    void loadDirectory(const std::string& utf8Dir);

    /// Idiomas encontrados, ordenados por código.
    const std::vector<Language>& languages() const { return languages_; }

    /// Cambia el idioma activo. @return false si el código no existe.
    bool setLanguage(const std::string& code);

    /// Código del idioma activo.
    const std::string& language() const { return current_; }

    /// Texto de una clave; si no existe devuelve la propia clave.
    std::string tr(const std::string& key) const;

    /// Texto de una clave; si no existe devuelve @p fallback.
    std::string tr(const std::string& key, const std::string& fallback) const;

private:
    std::map<std::string, std::map<std::string, std::string>> tables_;
    std::vector<Language> languages_;
    std::string current_;
};

} // namespace cvgui
