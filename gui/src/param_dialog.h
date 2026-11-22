/**
 * @file param_dialog.h
 * @brief Diálogo genérico para editar los parámetros de una operación.
 */
#pragma once

#include <gtk/gtk.h>

#include <opencv2/core.hpp>

#include "cvcore/operation.h"
#include "cvgui/i18n.h"

namespace cvgui {

/**
 * @brief Muestra un diálogo con un control por parámetro.
 *
 * El diálogo se genera a partir de Operation::params(), así que un módulo
 * nuevo no necesita código de interfaz. Si se le pasa la imagen, muestra
 * además una vista previa con reglas en píxeles que se recalcula al cambiar
 * un parámetro (aplicando la operación a la imagen completa).
 *
 * @param parent Ventana principal.
 * @param i18n   Traducciones (claves `op.<id>`, `op.<id>.<param>` y
 *               `op.<id>.<param>.<opción>`; si faltan se usa el texto del módulo).
 * @param op     Operación a configurar.
 * @param values Entrada: valores iniciales. Salida: valores elegidos.
 * @param image  Imagen para la vista previa (vacía: sin vista previa).
 * @param result Si no es nulo y la vista previa se calculó con los valores
 *               elegidos, recibe ese resultado para no repetir el cálculo.
 * @return true si el usuario aceptó.
 */
bool runParamDialog(GtkWindow* parent, const I18n& i18n, const cvcore::Operation& op,
                    cvcore::ParamValues& values, const cv::Mat& image = cv::Mat(), cv::Mat* result = nullptr);

} // namespace cvgui
