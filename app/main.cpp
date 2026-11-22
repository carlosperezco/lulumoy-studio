/**
 * @file main.cpp
 * @brief Gestión central: localiza los recursos, carga configuración e
 *        idiomas y lanza la interfaz (o el modo de línea de comandos).
 *
 * Uso:
 * @code
 * lulumoy [imagen]                              Abre la interfaz gráfica.
 * lulumoy --apply cadena.cvchain entrada salida Aplica una cadena sin interfaz.
 * lulumoy --version                             Muestra la versión.
 * lulumoy --list                                Lista las operaciones disponibles.
 * lulumoy --info imagen                         Muestra propiedades y metadatos.
 * lulumoy --svg entrada salida.svg [clave=valor]  Vectoriza en blanco y negro.
 * @endcode
 */
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "cvcore/image_info.h"
#include "cvcore/image_io.h"
#include "cvcore/pipeline.h"
#include "cvcore/registry.h"
#include "cvcore/vectorize.h"
#include "lulumoy_version.h"
#include "cvgui/app_config.h"
#include "cvgui/gui.h"
#include "cvgui/i18n.h"

namespace fs = std::filesystem;

namespace {

/// Carpeta donde está el ejecutable.
fs::path executableDir(const char* argv0) {
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);
    const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (n > 0 && n < buffer.size()) return fs::path(std::wstring(buffer.data(), n)).parent_path();
#else
    std::error_code ec;
    const fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return self.parent_path();
#endif
    std::error_code ec2;
    return fs::absolute(argv0 ? argv0 : ".", ec2).parent_path();
}

/**
 * @brief Busca la carpeta de recursos (la que contiene `lang/`).
 *
 * Orden: variable de entorno LULUMOY_HOME, carpeta del ejecutable,
 * `../share/lulumoy` (instalación en Linux) y carpeta actual.
 */
fs::path findResourceDir(const fs::path& exeDir) {
    std::vector<fs::path> candidates;
    if (const char* home = std::getenv("LULUMOY_HOME")) candidates.emplace_back(fs::u8path(home));
    candidates.push_back(exeDir);
    candidates.push_back(exeDir / ".." / "share" / "lulumoy");
    std::error_code ec;
    candidates.push_back(fs::current_path(ec));

    for (const fs::path& dir : candidates) {
        if (fs::is_directory(dir / "lang", ec)) return dir;
    }
    return exeDir;
}

/**
 * @brief Carpeta de ajustes del usuario.
 *
 * Windows: %APPDATA%\\DonatellaTech\\Lulumoy Studio.
 * Linux: $XDG_CONFIG_HOME/donatellatech/lulumoy o ~/.config/donatellatech/lulumoy.
 * @return Ruta vacía si no se puede determinar.
 */
fs::path userSettingsDir() {
#ifdef _WIN32
    if (const wchar_t* appdata = ::_wgetenv(L"APPDATA")) {
        if (*appdata) return fs::path(appdata) / L"DonatellaTech" / L"Lulumoy Studio";
    }
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdg) return fs::u8path(xdg) / "donatellatech" / "lulumoy";
    }
    if (const char* home = std::getenv("HOME")) {
        if (*home) return fs::u8path(home) / ".config" / "donatellatech" / "lulumoy";
    }
#endif
    return fs::path();
}

/// Modo sin interfaz: aplica una cadena guardada a una imagen.
int applyChain(const std::string& chainPath, const std::string& input, const std::string& output) {
    try {
        const cvcore::Pipeline chain = cvcore::Pipeline::load(chainPath);
        const cv::Mat result = chain.run(cvcore::loadImage(input));
        cvcore::saveImageNew(output, result);
        std::cout << "OK: " << output << " (" << chain.size() << " steps)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

/// Lista las operaciones registradas y sus parámetros.
int listOperations() {
    for (const cvcore::Operation* op : cvcore::Registry::instance().all()) {
        std::cout << op->id() << "  [" << op->menu() << "]  " << op->label() << "\n";
        for (const cvcore::ParamSpec& p : op->params()) {
            std::cout << "    " << p.key << " = " << cvcore::formatNumber(p.def) << "   (" << p.label << ")\n";
        }
    }
    return 0;
}

/// Modo sin interfaz: propiedades y metadatos de una imagen.
int showInfo(const std::string& path) {
    std::vector<cvcore::InfoItem> items = cvcore::fileInfo(path);
    try {
        const std::vector<cvcore::InfoItem> image = cvcore::imageInfo(cvcore::loadImage(path));
        items.insert(items.end(), image.begin(), image.end());
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    std::cout << cvcore::infoToText(items);
    return 0;
}

/**
 * @brief Modo sin interfaz: vectoriza una imagen y la guarda como SVG nuevo.
 * @param args Parámetros "clave=valor" (ver vectorizeParams()).
 */
int exportSvg(const std::string& input, const std::string& output, const std::vector<std::string>& args) {
    try {
        cvcore::ParamValues values = cvcore::defaultValues(cvcore::vectorizeParams());
        for (const std::string& arg : args) {
            const std::size_t eq = arg.find('=');
            double value = 0.0;
            if (eq == std::string::npos || !cvcore::parseNumber(arg.substr(eq + 1), value)) {
                std::cerr << "Error: invalid parameter " << arg << " (use key=value)\n";
                return 2;
            }
            values[arg.substr(0, eq)] = value;
        }
        const cvcore::VectorResult result = cvcore::vectorize(cvcore::loadImage(input), values);
        cvcore::saveSvgNew(output, cvcore::toSvg(result));
        std::cout << "OK: " << output << " (" << result.paths.size() << " paths, " << result.nodes() << " nodes)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace

int main(int argc, char** argv) {
    const std::string first = argc > 1 ? argv[1] : "";
    if (first == "--version") {
        std::cout << "Lulumoy Studio " << LULUMOY_VERSION << "\n";
        return 0;
    }
    if (first == "--list") return listOperations();
    if (first == "--info") {
        if (argc != 3) {
            std::cerr << "Usage: lulumoy --info image\n";
            return 2;
        }
        return showInfo(argv[2]);
    }
    if (first == "--svg") {
        if (argc < 4) {
            std::cerr << "Usage: lulumoy --svg input output.svg [key=value ...]\nParameters:\n";
            for (const cvcore::ParamSpec& p : cvcore::vectorizeParams()) {
                std::cerr << "    " << p.key << " = " << cvcore::formatNumber(p.def) << "   (" << p.label << ")\n";
            }
            return 2;
        }
        return exportSvg(argv[2], argv[3], std::vector<std::string>(argv + 4, argv + argc));
    }
    if (first == "--apply") {
        if (argc != 5) {
            std::cerr << "Usage: lulumoy --apply chain.cvchain input output\n";
            return 2;
        }
        return applyChain(argv[2], argv[3], argv[4]);
    }

    const fs::path resources = findResourceDir(executableDir(argc > 0 ? argv[0] : nullptr));

    cvgui::AppConfig config;
    config.load(cvcore::toUtf8(resources / "config" / "app.conf"));
    const fs::path userDir = userSettingsDir();
    if (!userDir.empty()) config.loadUserSettings(cvcore::toUtf8(userDir / "settings.conf"));

    cvgui::I18n i18n;
    i18n.loadDirectory(cvcore::toUtf8(resources / "lang"));
    i18n.setLanguage(config.get("language", "es"));

    // Un primer argumento que no sea una opción es la imagen a abrir.
    const std::string initialImage = (!first.empty() && first[0] != '-') ? first : "";
    return cvgui::runGui(argc, argv, config, i18n, cvcore::toUtf8(resources), initialImage);
}
