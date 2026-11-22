/**
 * @file image_io.h
 * @brief Lectura y escritura de imágenes y utilidades comunes de imagen.
 */
#pragma once

#include <filesystem>
#include <string>

#include <opencv2/core.hpp>

namespace cvcore {

/// Convierte una ruta UTF-8 a std::filesystem::path (correcto también en Windows).
std::filesystem::path fromUtf8(const std::string& utf8Path);
/// Convierte una ruta a texto UTF-8.
std::string toUtf8(const std::filesystem::path& path);

/**
 * @brief Abre una imagen conservando su profundidad y sus canales.
 *
 * El resultado es siempre de 8 o 16 bits sin signo con 1, 3 o 4 canales;
 * otros tipos (p. ej. coma flotante) se escalan a 16 bits.
 * @throws cvcore::Error si no se puede leer o decodificar.
 */
cv::Mat loadImage(const std::string& utf8Path);

/**
 * @brief Guarda una imagen en un archivo NUEVO.
 *
 * Nunca sobrescribe: si el archivo ya existe lanza un error con código
 * "file_exists". El formato se decide por la extensión.
 * @throws cvcore::Error
 */
void saveImageNew(const std::string& utf8Path, const cv::Mat& image);

/**
 * @brief Propone un nombre de archivo que todavía no existe.
 *
 * Para `foto.png` devuelve `foto_edit.png`, `foto_edit_2.png`, etc.
 */
std::string suggestNewPath(const std::string& utf8Path);

/// Convierte cualquier imagen de la aplicación a RGB de 8 bits para mostrarla.
cv::Mat toDisplayRgb(const cv::Mat& image);

/// Convierte a escala de grises (1 canal) conservando la profundidad.
cv::Mat toGray(const cv::Mat& image);

/// Valor máximo del tipo de la imagen: 255 (8 bits) o 65535 (16 bits).
double maxValue(const cv::Mat& image);

/// Ajusta un tamaño de núcleo al impar más cercano por arriba (mínimo 1).
int oddKernel(int size);

} // namespace cvcore
