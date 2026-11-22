/**
 * @file vectorize.h
 * @brief Vectorización en blanco y negro: de una imagen a trazados SVG.
 *
 * Proceso:
 * 1. Escala de grises de 8 bits y, opcionalmente, ampliación (resolución de trabajo).
 * 2. Desenfoque previo opcional y binarización (umbral fijo, Otsu o adaptativo).
 * 3. Limpieza morfológica opcional (apertura y cierre).
 * 4. Contornos con cv::findContours (exteriores y agujeros).
 * 5. Cada contorno se desplaza medio píxel hacia el fondo para que siga el
 *    borde real de los píxeles (sin esto las formas adelgazan un píxel y
 *    las líneas de 1 px desaparecen).
 * 6. Simplificación con cv::approxPolyDP y, opcionalmente, curvas de Bézier
 *    cuadráticas que conservan las esquinas marcadas.
 *
 * Las coordenadas de los trazados están en píxeles de la imagen original,
 * con el origen en la esquina superior izquierda del primer píxel (la misma
 * convención que SVG).
 */
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "cvcore/params.h"

namespace cvcore {

/// Tramo de un trazado: recta hasta @c end o curva cuadrática con punto de control @c control.
struct VectorSegment {
    bool curve = false;
    cv::Point2f control; ///< Solo si curve es true.
    cv::Point2f end;
};

/// Trazado cerrado: empieza en @c start y vuelve a él al final.
struct VectorPath {
    cv::Point2f start;
    std::vector<VectorSegment> segments;
};

/// Resultado de la vectorización.
struct VectorResult {
    int width = 0;                 ///< Ancho de la imagen original (px).
    int height = 0;                ///< Alto de la imagen original (px).
    int workScale = 1;             ///< Factor de ampliación usado realmente (1, 2 o 4).
    bool background = false;       ///< El SVG incluye un fondo blanco.
    std::vector<VectorPath> paths; ///< Formas y agujeros (se rellenan con la regla par-impar).
    cv::Mat binary;                ///< Imagen binarizada (255 = forma), a la resolución de trabajo.

    /// Número total de tramos (nodos) de todos los trazados.
    std::size_t nodes() const;
};

/**
 * @brief Parámetros configurables de la vectorización.
 *
 * Se describen con ParamSpec para que la interfaz genere los controles
 * igual que para las operaciones (traducción con `vec.<clave>`).
 */
std::vector<ParamSpec> vectorizeParams();

/**
 * @brief Vectoriza una imagen.
 * @param image  Imagen de la aplicación (8 o 16 bits, 1, 3 o 4 canales).
 * @param values Valores de vectorizeParams() (se validan con sanitize()).
 * @throws cvcore::Error si la imagen está vacía.
 */
VectorResult vectorize(const cv::Mat& image, const ParamValues& values);

/**
 * @brief Contornos tal como se dibujan: las curvas se convierten en polilíneas.
 * @param samples Segmentos rectos por cada curva.
 */
std::vector<std::vector<cv::Point2f>> outlines(const VectorResult& result, int samples = 8);

/**
 * @brief Dibuja el resultado como máscara (CV_8UC1, 255 = relleno) para la vista previa.
 *
 * Usa sobremuestreo, así que los bordes salen suavizados y coinciden con lo
 * que dibuja un visor de SVG.
 * @param scale Escala respecto a la imagen original (1 = mismo tamaño).
 */
cv::Mat renderMask(const VectorResult& result, double scale);

/// Texto SVG del resultado (relleno negro, regla par-impar).
std::string toSvg(const VectorResult& result);

/**
 * @brief Guarda un SVG en un archivo NUEVO; nunca sobrescribe.
 * @throws cvcore::Error con código "file_exists" o "write_failed".
 */
void saveSvgNew(const std::string& utf8Path, const std::string& svg);

} // namespace cvcore
