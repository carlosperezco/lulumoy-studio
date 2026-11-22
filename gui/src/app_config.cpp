/**
 * @file app_config.cpp
 * @brief Implementación de la configuración de la aplicación.
 */
#include "cvgui/app_config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include "cvcore/image_io.h"
#include "kvfile.h"

namespace cvgui {

bool AppConfig::load(const std::string& utf8Path) {
    path_ = utf8Path;
    values_.clear();
    dirty_.clear();
    const auto pairs = readKeyValueFile(utf8Path);
    for (const auto& kv : pairs) values_[kv.first] = kv.second;
    return !pairs.empty();
}

bool AppConfig::loadUserSettings(const std::string& utf8Path) {
    path_ = utf8Path;
    dirty_.clear();
    const auto pairs = readKeyValueFile(utf8Path);
    for (const auto& kv : pairs) values_[kv.first] = kv.second;
    return !pairs.empty();
}

bool AppConfig::has(const std::string& key) const {
    auto it = values_.find(key);
    return it != values_.end() && !it->second.empty();
}

std::string AppConfig::get(const std::string& key, const std::string& fallback) const {
    auto it = values_.find(key);
    return (it != values_.end() && !it->second.empty()) ? it->second : fallback;
}

int AppConfig::getInt(const std::string& key, int fallback) const {
    auto it = values_.find(key);
    if (it == values_.end() || it->second.empty()) return fallback;
    char* end = nullptr;
    const long v = std::strtol(it->second.c_str(), &end, 10);
    return (end && *end == '\0') ? static_cast<int>(v) : fallback;
}

void AppConfig::set(const std::string& key, const std::string& value) {
    values_[key] = value;
    dirty_.insert(key);
}

bool AppConfig::save() {
    if (path_.empty()) return false;
    if (dirty_.empty()) return true;

    std::vector<std::string> lines;
    {
        std::ifstream in(cvcore::fromUtf8(path_), std::ios::binary);
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
    }

    std::set<std::string> pending = dirty_;
    for (std::string& line : lines) {
        std::string key, value;
        if (splitKeyValue(line, key, value) && pending.count(key)) {
            line = key + " = " + values_[key];
            pending.erase(key);
        }
    }
    for (const std::string& key : pending) lines.push_back(key + " = " + values_[key]);

    const std::filesystem::path file = cvcore::fromUtf8(path_);
    std::error_code ec;
    if (file.has_parent_path()) std::filesystem::create_directories(file.parent_path(), ec);
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    for (const std::string& line : lines) out << line << '\n';
    if (!out) return false;
    dirty_.clear();
    return true;
}

} // namespace cvgui
