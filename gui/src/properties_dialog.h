/**
 * @file properties_dialog.h
 * @brief Diálogo "Propiedades": tabla de datos y metadatos de la imagen.
 */
#pragma once

#include <string>
#include <vector>

#include <gtk/gtk.h>

#include "cvcore/image_info.h"
#include "cvgui/i18n.h"

namespace cvgui {

/**
 * @brief Muestra las propiedades agrupadas en una tabla con desplazamiento.
 *
 * Los nombres se traducen con `prop.<key>` y los grupos con
 * `prop.group.<group>`; si falta la clave se usa el texto del propio
 * elemento. Los valores se pueden seleccionar con el ratón y el botón
 * "Copiar todo" pone la tabla completa en el portapapeles.
 *
 * @param parent Ventana principal.
 * @param i18n   Traducciones.
 * @param title  Título del diálogo.
 * @param items  Propiedades, ya ordenadas por grupo.
 */
void runPropertiesDialog(GtkWindow* parent, const I18n& i18n, const std::string& title,
                         const std::vector<cvcore::InfoItem>& items);

} // namespace cvgui
