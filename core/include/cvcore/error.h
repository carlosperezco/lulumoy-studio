/**
 * @file error.h
 * @brief Excepción común de la librería de operaciones.
 */
#pragma once

#include <stdexcept>
#include <string>

namespace cvcore {

/**
 * @brief Error con un código estable que la interfaz puede traducir.
 *
 * El texto de what() está en inglés y sirve como mensaje por defecto. La
 * interfaz busca la clave `err.<code>` en el idioma activo y, si existe,
 * la usa en su lugar.
 */
class Error : public std::runtime_error {
public:
    /**
     * @param code   Código estable (p. ej. "file_exists").
     * @param message Mensaje por defecto en inglés.
     * @param detail  Dato variable (ruta, nombre de operación...). Opcional.
     */
    Error(std::string code, const std::string& message, std::string detail = "")
        : std::runtime_error(detail.empty() ? message : message + ": " + detail),
          code_(std::move(code)),
          message_(message),
          detail_(std::move(detail)) {}

    /// Código estable del error.
    const std::string& code() const noexcept { return code_; }
    /// Mensaje por defecto, sin el detalle.
    const std::string& message() const noexcept { return message_; }
    /// Dato variable asociado al error (puede estar vacío).
    const std::string& detail() const noexcept { return detail_; }

private:
    std::string code_;
    std::string message_;
    std::string detail_;
};

} // namespace cvcore
