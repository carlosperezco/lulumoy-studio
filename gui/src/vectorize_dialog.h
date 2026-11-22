/**
 * @file vectorize_dialog.h
 * @brief Diálogo de vectorización con vista previa.
 */
#pragma once

#include <gtk/gtk.h>

#include <opencv2/core.hpp>

#include "cvcore/params.h"
#include "cvgui/i18n.h"

namespace cvgui {

/**
 * @brief Muestra los parámetros de vectorización junto a una vista previa.
 *
 * La vista previa se recalcula poco después de cada cambio (con una pausa
 * para no repetir el cálculo mientras se pulsan las flechas) y se puede ver
 * como resultado vectorial, superpuesta a la imagen o como imagen
 * binarizada. Debajo se indica el número de trazados, nodos y el tamaño
 * aproximado del SVG.
 *
 * @param parent Ventana principal.
 * @param i18n   Traducciones (claves `vec.*` y `vecdlg.*`).
 * @param image  Imagen a vectorizar.
 * @param values Entrada: valores iniciales. Salida: valores elegidos.
 * @return true si el usuario pulsó "Guardar SVG".
 */
bool runVectorizeDialog(GtkWindow* parent, const I18n& i18n, const cv::Mat& image, cvcore::ParamValues& values);

} // namespace cvgui
