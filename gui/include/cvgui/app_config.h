/**
 * @file app_config.h
 * @brief Configuración de la aplicación (título, icono, logo, mensaje, idioma).
 */
#pragma once

#include <map>
#include <set>
#include <string>

namespace cvgui {

/**
 * @brief Archivo de configuración `clave = valor`.
 *
 * Claves reconocidas:
 * - `window_title`: nombre de la ventana.
 * - `icon`: imagen del icono de la ventana.
 * - `welcome_logo`: logo de la pantalla inicial.
 * - `welcome_message`: mensaje de la pantalla inicial. Se puede dar uno
 *   por idioma con `welcome_message.es`, `welcome_message.en`, etc.
 * - `language`: idioma inicial ("es", "en", "it").
 * - `window_width`, `window_height`: tamaño inicial de la ventana.
 *
 * Las rutas relativas se resuelven respecto a la carpeta de recursos.
 *
 * Los valores por defecto se leen con load() del archivo del programa y los
 * del usuario con loadUserSettings(), que tienen prioridad. Lo que se cambia
 * desde la aplicación (el idioma) se guarda en el archivo del usuario, porque
 * la carpeta del programa puede no admitir escritura (por ejemplo, en
 * "Archivos de programa").
 */
class AppConfig {
public:
    /// Carga el archivo (ruta UTF-8). @return false si no existe; se usan los valores por defecto.
    bool load(const std::string& utf8Path);

    /**
     * @brief Carga los ajustes del usuario por encima de los ya cargados.
     *
     * A partir de esta llamada, save() escribe en este archivo; si no existe,
     * se crea (junto con sus carpetas) al guardar.
     * @return false si el archivo todavía no existe.
     */
    bool loadUserSettings(const std::string& utf8Path);

    /// @return true si la clave existe y no está vacía.
    bool has(const std::string& key) const;

    /// Valor de una clave o @p fallback si no existe.
    std::string get(const std::string& key, const std::string& fallback = "") const;

    /// Valor entero de una clave o @p fallback si no existe o no es un número.
    int getInt(const std::string& key, int fallback) const;

    /// Cambia un valor en memoria (se escribe con save()).
    void set(const std::string& key, const std::string& value);

    /**
     * @brief Escribe las claves cambiadas con set().
     *
     * Escribe en el archivo de usuario si se cargó con loadUserSettings();
     * si no, en el de load(). Conserva los comentarios y el resto de líneas.
     * @return false si no se puede escribir.
     */
    bool save();

private:
    std::string path_;
    std::map<std::string, std::string> values_;
    std::set<std::string> dirty_;
};

} // namespace cvgui
