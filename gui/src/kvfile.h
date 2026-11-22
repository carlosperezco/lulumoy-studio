/**
 * @file kvfile.h
 * @brief Lector de archivos de texto `clave = valor` (configuración e idiomas).
 */
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace cvgui {

/// Quita espacios, tabuladores y saltos de línea de los extremos.
std::string trim(const std::string& text);

/**
 * @brief Separa una línea `clave = valor`.
 * @return false si la línea está vacía, es un comentario (#) o no tiene '='.
 */
bool splitKeyValue(const std::string& line, std::string& key, std::string& value);

/**
 * @brief Lee un archivo `clave = valor` en UTF-8.
 *
 * Las líneas vacías y las que empiezan por '#' se ignoran. En los valores,
 * la secuencia `\n` se convierte en un salto de línea.
 * @return Pares en el orden del archivo; vacío si no se puede abrir.
 */
std::vector<std::pair<std::string, std::string>> readKeyValueFile(const std::string& utf8Path);

} // namespace cvgui
