/**
 * @file params.h
 * @brief Descripción y valores de los parámetros de una operación.
 *
 * Todos los valores se guardan como `double` (un booleano es 0/1, una lista
 * de opciones es el índice elegido y un punto son dos valores, `clave.x` y
 * `clave.y`, en píxeles de la imagen). Así la interfaz puede generar
 * el diálogo de parámetros sin conocer la operación y las cadenas de
 * comandos se guardan en texto plano sin librerías adicionales.
 */
#pragma once

#include <map>
#include <string>
#include <vector>

namespace cvcore {

/**
 * @brief Tipo de control que necesita un parámetro.
 *
 * `Point` no aparece en el diálogo de parámetros: la interfaz lo pide con
 * un clic del ratón sobre la imagen, en el orden en que se declaran.
 */
enum class ParamType { Int, Double, Bool, Choice, Point };

/// Descripción de un parámetro configurable de una operación.
struct ParamSpec {
    std::string key;    ///< Identificador (sin espacios); se usa en los archivos de cadena.
    std::string label;  ///< Etiqueta por defecto en inglés (traducible con `op.<id>.<key>`).
    ParamType type = ParamType::Double;
    double def = 0.0;   ///< Valor por defecto.
    double min = 0.0;   ///< Mínimo (Int/Double).
    double max = 0.0;   ///< Máximo (Int/Double).
    double step = 1.0;  ///< Incremento del control (Int/Double).
    std::vector<std::string> choices; ///< Opciones (solo Choice).
    std::string linkTo; ///< Solo Point: clave del punto del que este es destino (se dibuja una flecha).

    /// Parámetro entero.
    static ParamSpec integer(std::string key, std::string label, int def, int min, int max, int step = 1);
    /// Parámetro real.
    static ParamSpec real(std::string key, std::string label, double def, double min, double max, double step);
    /// Parámetro sí/no.
    static ParamSpec boolean(std::string key, std::string label, bool def);
    /// Parámetro de lista de opciones; el valor es el índice de la opción.
    static ParamSpec choice(std::string key, std::string label, std::vector<std::string> choices, int def = 0);
    /**
     * @brief Punto de la imagen elegido con el ratón.
     *
     * Produce dos valores, `<key>.x` y `<key>.y`, en píxeles de la imagen.
     * @param linkTo Clave de otro punto del que este es el destino; vacío si es independiente.
     */
    static ParamSpec point(std::string key, std::string label, std::string linkTo = "");
};

/// Valores de los parámetros: clave -> valor numérico.
using ParamValues = std::map<std::string, double>;

/// Devuelve los valores por defecto de una lista de parámetros.
ParamValues defaultValues(const std::vector<ParamSpec>& specs);

/**
 * @brief Completa y limita unos valores según su descripción.
 *
 * Añade los parámetros que falten con su valor por defecto, descarta los
 * desconocidos y recorta cada valor a su rango válido.
 */
ParamValues sanitize(const std::vector<ParamSpec>& specs, const ParamValues& values);

/// Lee un parámetro real (0 si no existe).
double getReal(const ParamValues& values, const std::string& key);
/// Lee un parámetro entero (redondeado).
int getInt(const ParamValues& values, const std::string& key);
/// Lee un parámetro booleano.
bool getBool(const ParamValues& values, const std::string& key);

/// Convierte un número a texto con punto decimal, sin depender del idioma del sistema.
std::string formatNumber(double value);
/// Convierte texto a número con punto decimal. @return false si el texto no es un número.
bool parseNumber(const std::string& text, double& value);

} // namespace cvcore
