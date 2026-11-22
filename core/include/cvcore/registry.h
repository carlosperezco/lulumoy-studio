/**
 * @file registry.h
 * @brief Registro de operaciones: los módulos se dan de alta solos.
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cvcore/operation.h"

namespace cvcore {

/**
 * @brief Catálogo de todas las operaciones disponibles.
 *
 * Es el único punto de contacto entre los módulos y el resto del
 * programa: la interfaz lo recorre para construir los menús y las
 * cadenas de comandos lo consultan para ejecutar cada paso.
 */
class Registry {
public:
    /// Acceso al registro único.
    static Registry& instance();

    /// Da de alta una operación. Si el id ya existe, la nueva lo reemplaza.
    void add(std::unique_ptr<Operation> op);

    /// Busca una operación por id. @return nullptr si no existe.
    const Operation* find(const std::string& id) const;

    /// Todas las operaciones, ordenadas por menú, orden e id.
    std::vector<const Operation*> all() const;

private:
    Registry() = default;
    std::map<std::string, std::unique_ptr<Operation>> ops_;
};

/// Objeto auxiliar cuya construcción registra una operación de tipo T.
template <class T>
struct Registrar {
    Registrar() { Registry::instance().add(std::make_unique<T>()); }
};

} // namespace cvcore

/**
 * @brief Registra una clase derivada de cvcore::Operation.
 *
 * Se escribe una vez, al final del .cpp del módulo:
 * @code
 * CVCORE_REGISTER_OPERATION(MiOperacion)
 * @endcode
 */
#define CVCORE_REGISTER_OPERATION(T) static ::cvcore::Registrar<T> cvcore_registrar_##T;
