/**
 * @file image_info.h
 * @brief Propiedades de una imagen: archivo, formato, metadatos y estadísticas.
 */
#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace cvcore {

/**
 * @brief Una propiedad (una fila del diálogo Propiedades).
 *
 * La interfaz traduce el nombre con la clave `prop.<key>` y el grupo con
 * `prop.group.<group>`; si no hay traducción usa @ref label.
 */
struct InfoItem {
    std::string group; ///< Grupo: "file", "format", "exif", "gps", "text" o "image".
    std::string key;   ///< Identificador estable, p. ej. "exif.model".
    std::string label; ///< Nombre por defecto en inglés.
    std::string value; ///< Valor ya convertido a texto.
};

/**
 * @brief Propiedades del archivo en disco.
 *
 * Lee directamente el archivo (sin OpenCV ni librerías externas):
 * - grupo "file": nombre, carpeta, tamaño y fecha de modificación;
 * - grupo "format": formato real según su cabecera (no la extensión),
 *   dimensiones, profundidad, compresión, resolución, perfil de color...;
 * - grupos "exif" y "gps": metadatos EXIF de JPEG, PNG, TIFF y WebP;
 * - grupo "text": textos incrustados (comentario JPEG, tEXt/iTXt de PNG).
 *
 * Nunca lanza excepciones: un archivo dañado o truncado simplemente
 * produce menos propiedades.
 */
std::vector<InfoItem> fileInfo(const std::string& utf8Path);

/**
 * @brief Propiedades de la imagen en memoria (grupo "image").
 *
 * Dimensiones, canales, profundidad, tipo OpenCV, memoria ocupada y
 * estadísticas (mínimo, máximo, media y desviación por canal).
 */
std::vector<InfoItem> imageInfo(const cv::Mat& image);

/// Convierte las propiedades a texto plano (una por línea, agrupadas).
std::string infoToText(const std::vector<InfoItem>& items);

} // namespace cvcore
