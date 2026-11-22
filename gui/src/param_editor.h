/**
 * @file param_editor.h
 * @brief Tabla de controles generada a partir de una lista de ParamSpec.
 */
#pragma once

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

#include "cvcore/params.h"
#include "cvgui/i18n.h"

namespace cvgui {

/**
 * @brief Crea un control por parámetro (número, casilla o lista) en una tabla.
 *
 * Los parámetros de tipo punto no tienen control: se conservan los valores
 * iniciales. Las etiquetas se traducen con `<prefijo>.<clave>` y las
 * opciones con `<prefijo>.<clave>.<opción>`; si falta la clave se usa el
 * texto de ParamSpec.
 *
 * El objeto debe vivir mientras exista su tabla, porque los controles
 * avisan de los cambios a través de él. No se puede copiar.
 */
class ParamEditor {
public:
    /**
     * @param i18n    Traducciones.
     * @param prefix  Prefijo de las claves de traducción (p. ej. "op.edit.rotate" o "vec").
     * @param specs   Parámetros.
     * @param initial Valores iniciales.
     */
    ParamEditor(const I18n& i18n, std::string prefix, std::vector<cvcore::ParamSpec> specs,
                const cvcore::ParamValues& initial);
    ParamEditor(const ParamEditor&) = delete;
    ParamEditor& operator=(const ParamEditor&) = delete;

    /// Tabla con los controles, para insertarla en un diálogo.
    GtkWidget* widget() const { return grid_; }

    /// @return true si hay al menos un control (algún parámetro que no sea un punto).
    bool hasControls() const;

    /// Confirma el texto escrito a mano en los campos numéricos.
    void commit();

    /// Valores actuales, ya validados con sanitize().
    cvcore::ParamValues values() const;

    /// Función que se llama cada vez que el usuario cambia un control.
    void setOnChanged(std::function<void()> callback) { onChanged_ = std::move(callback); }

private:
    static void changed(GtkWidget* widget, gpointer self);

    std::vector<cvcore::ParamSpec> specs_;
    cvcore::ParamValues initial_;
    std::vector<GtkWidget*> editors_; ///< Uno por parámetro (nullptr para los puntos).
    GtkWidget* grid_ = nullptr;
    std::function<void()> onChanged_;
};

} // namespace cvgui
