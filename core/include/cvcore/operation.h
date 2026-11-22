/**
 * @file operation.h
 * @brief Interfaz que implementa cada módulo de edición.
 */
#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "cvcore/params.h"

namespace cvcore {

/**
 * @brief Una operación sobre una imagen (un módulo).
 *
 * Para añadir una funcionalidad se crea un archivo .cpp en `core/modules`
 * con una clase derivada y la macro CVCORE_REGISTER_OPERATION. No hay que
 * tocar la interfaz gráfica: el menú se construye a partir del registro.
 *
 * Las imágenes que circulan por la aplicación son siempre de 8 o 16 bits
 * sin signo, con 1 (grises), 3 (BGR) o 4 (BGRA) canales.
 */
class Operation {
public:
    virtual ~Operation() = default;

    /// Identificador único y estable, p. ej. "edit.rotate". Se guarda en las cadenas.
    virtual std::string id() const = 0;

    /**
     * @brief Menú donde aparece, con '/' para submenús.
     *
     * Ejemplos: "edit", "edit/filters", "edit/format". Si el menú no
     * existe, la interfaz lo crea (se traduce con `menu.<ruta>`).
     */
    virtual std::string menu() const = 0;

    /// Nombre por defecto en inglés (se traduce con `op.<id>`).
    virtual std::string label() const = 0;

    /// Posición dentro de su menú (menor = antes).
    virtual int order() const { return 100; }

    /// Parámetros configurables. Vacío si la operación no tiene.
    virtual std::vector<ParamSpec> params() const { return {}; }

    /**
     * @brief Aplica la operación.
     * @param input  Imagen de entrada (no se modifica).
     * @param values Valores ya validados con sanitize().
     * @return Imagen nueva.
     * @throws cvcore::Error o cv::Exception si no se puede aplicar.
     */
    virtual cv::Mat apply(const cv::Mat& input, const ParamValues& values) const = 0;

    /**
     * @brief Zona de la imagen de entrada que conviene marcar en la vista previa (opcional).
     *
     * La vista previa de los diálogos la dibuja como un polígono cerrado sobre
     * la imagen original, por ejemplo el rectángulo que se conserva al
     * recortar o los puntos de origen de una transformación.
     *
     * @param input  Tamaño de la imagen de entrada.
     * @param values Valores ya validados con sanitize().
     * @return Vértices en coordenadas de la imagen de entrada; vacío si no hay nada que marcar.
     */
    virtual std::vector<cv::Point2f> region(const cv::Size& input, const ParamValues& values) const {
        (void)input;
        (void)values;
        return {};
    }
};

} // namespace cvcore
