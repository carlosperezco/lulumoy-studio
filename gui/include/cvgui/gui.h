/**
 * @file gui.h
 * @brief Punto de entrada de la interfaz gráfica (GTK 3).
 */
#pragma once

#include <string>

#include "cvgui/app_config.h"
#include "cvgui/i18n.h"

namespace cvgui {

/**
 * @brief Crea la ventana principal y ejecuta el bucle de eventos.
 *
 * @param argc,argv     Argumentos del programa (GTK consume los suyos).
 * @param config        Configuración ya cargada.
 * @param i18n          Traducciones ya cargadas.
 * @param resourceDir   Carpeta con `config/`, `lang/` y `assets/` (UTF-8).
 * @param initialImage  Imagen que se abre al arrancar; vacío para ninguna.
 * @return Código de salida del programa.
 */
int runGui(int argc, char** argv, AppConfig& config, I18n& i18n, const std::string& resourceDir,
           const std::string& initialImage);

} // namespace cvgui
