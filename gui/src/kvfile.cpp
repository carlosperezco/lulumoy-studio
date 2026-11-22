/**
 * @file kvfile.cpp
 * @brief Implementación del lector `clave = valor`.
 */
#include "kvfile.h"

#include <fstream>

#include "cvcore/image_io.h"

namespace cvgui {

std::string trim(const std::string& text) {
    const char* const kSpaces = " \t\r\n";
    const std::size_t first = text.find_first_not_of(kSpaces);
    if (first == std::string::npos) return "";
    const std::size_t last = text.find_last_not_of(kSpaces);
    return text.substr(first, last - first + 1);
}

bool splitKeyValue(const std::string& line, std::string& key, std::string& value) {
    const std::string t = trim(line);
    if (t.empty() || t[0] == '#') return false;
    const std::size_t eq = t.find('=');
    if (eq == std::string::npos || eq == 0) return false;
    key = trim(t.substr(0, eq));
    value = trim(t.substr(eq + 1));
    return !key.empty();
}

std::vector<std::pair<std::string, std::string>> readKeyValueFile(const std::string& utf8Path) {
    std::vector<std::pair<std::string, std::string>> pairs;
    std::ifstream in(cvcore::fromUtf8(utf8Path), std::ios::binary);
    if (!in) return pairs;

    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (first) {
            first = false;
            if (line.rfind("\xEF\xBB\xBF", 0) == 0) line.erase(0, 3); // BOM UTF-8
        }
        std::string key, value;
        if (!splitKeyValue(line, key, value)) continue;

        std::size_t pos = 0;
        while ((pos = value.find("\\n", pos)) != std::string::npos) {
            value.replace(pos, 2, "\n");
            ++pos;
        }
        pairs.emplace_back(std::move(key), std::move(value));
    }
    return pairs;
}

} // namespace cvgui
