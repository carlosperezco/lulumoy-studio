/**
 * @file i18n.cpp
 * @brief Implementación de las traducciones.
 */
#include "cvgui/i18n.h"

#include <algorithm>
#include <filesystem>

#include "cvcore/image_io.h"
#include "kvfile.h"

namespace cvgui {

namespace fs = std::filesystem;

void I18n::loadDirectory(const std::string& utf8Dir) {
    std::error_code ec;
    for (fs::directory_iterator it(cvcore::fromUtf8(utf8Dir), ec), end; !ec && it != end; it.increment(ec)) {
        const fs::path& file = it->path();
        if (file.extension() != ".lang") continue;

        const std::string code = cvcore::toUtf8(file.stem());
        std::map<std::string, std::string>& table = tables_[code];
        for (auto& kv : readKeyValueFile(cvcore::toUtf8(file))) table[kv.first] = kv.second;
    }

    languages_.clear();
    for (const auto& entry : tables_) {
        auto name = entry.second.find("lang.name");
        languages_.push_back({entry.first, name != entry.second.end() ? name->second : entry.first});
    }
    if (current_.empty() && !languages_.empty()) current_ = languages_.front().code;
}

bool I18n::setLanguage(const std::string& code) {
    if (tables_.find(code) == tables_.end()) return false;
    current_ = code;
    return true;
}

std::string I18n::tr(const std::string& key) const {
    return tr(key, key);
}

std::string I18n::tr(const std::string& key, const std::string& fallback) const {
    auto table = tables_.find(current_);
    if (table == tables_.end()) return fallback;
    auto text = table->second.find(key);
    return text != table->second.end() ? text->second : fallback;
}

} // namespace cvgui
