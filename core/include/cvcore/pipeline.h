/**
 * @file pipeline.h
 * @brief Cadena de comandos: lista ordenada de operaciones con sus parámetros.
 */
#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "cvcore/params.h"

namespace cvcore {

/// Un paso de la cadena.
struct Step {
    std::string opId;   ///< Id de la operación (ver Operation::id()).
    ParamValues params; ///< Parámetros con los que se aplicó.
};

/**
 * @brief Cadena de comandos reutilizable.
 *
 * Formato del archivo (texto, una operación por línea):
 * @code
 * # lulumoy-chain 1
 * edit.rotate angle=90 expand=1
 * filter.gaussian ksize=5 sigma=1.5
 * @endcode
 * Las líneas que empiezan por '#' son comentarios. Los parámetros que no
 * aparezcan toman su valor por defecto.
 */
class Pipeline {
public:
    /// Añade un paso al final.
    void add(Step step);
    /// Quita el último paso (no hace nada si está vacía).
    void removeLast();
    /// Vacía la cadena.
    void clear();
    /// @return true si no tiene pasos.
    bool empty() const { return steps_.empty(); }
    /// Número de pasos.
    std::size_t size() const { return steps_.size(); }
    /// Pasos de la cadena, en orden.
    const std::vector<Step>& steps() const { return steps_; }

    /**
     * @brief Aplica todos los pasos a una imagen.
     * @throws cvcore::Error si un paso no existe o falla.
     */
    cv::Mat run(const cv::Mat& input) const;

    /// Texto de la cadena en el formato de archivo.
    std::string toText() const;
    /// Construye una cadena a partir de texto. @throws cvcore::Error si hay una línea inválida.
    static Pipeline fromText(const std::string& text);

    /// Guarda la cadena en un archivo (ruta UTF-8). @throws cvcore::Error
    void save(const std::string& utf8Path) const;
    /// Carga una cadena desde un archivo (ruta UTF-8). @throws cvcore::Error
    static Pipeline load(const std::string& utf8Path);

private:
    std::vector<Step> steps_;
};

} // namespace cvcore
