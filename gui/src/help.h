/**
 * @file help.h
 * @brief Guía matemática: páginas HTML locales que se abren en el navegador.
 *
 * Las páginas están en `help/<idioma>/<página>.html`, una por operación, con
 * el mismo nombre que su id (p. ej. `filter.gaussian.html`), más `index.html`,
 * `vectorize.html`, `interpolation.html` y `format.html`. Las fórmulas están
 * escritas en MathML, que los navegadores actuales muestran sin conexión ni
 * librerías adicionales.
 */
#pragma once

#include <string>

#include <gtk/gtk.h>

#include "cvgui/i18n.h"

namespace cvgui {

/**
 * @brief Indica dónde están las páginas y qué idioma usar.
 * @param resourceDir Carpeta que contiene `help/` (UTF-8).
 * @param i18n        Traducciones; se consulta el idioma activo en cada apertura.
 */
void initHelp(const std::string& resourceDir, const I18n& i18n);

/// @return true si hay página para @p page en el idioma activo o en inglés.
bool hasHelp(const std::string& page);

/**
 * @brief Abre una página en el navegador predeterminado.
 *
 * Busca primero en el idioma activo y después en inglés; si no existe, abre
 * el índice. Si no se puede abrir, muestra un mensaje con la ruta.
 * @param parent Ventana sobre la que mostrar un posible error.
 * @param page   Nombre de la página sin extensión ("index", "filter.gaussian"...).
 */
void openHelp(GtkWindow* parent, const std::string& page);

} // namespace cvgui
