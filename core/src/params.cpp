/**
 * @file params.cpp
 * @brief Implementación de los parámetros de operación.
 */
#include "cvcore/params.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace cvcore {

ParamSpec ParamSpec::integer(std::string key, std::string label, int def, int min, int max, int step) {
    ParamSpec s;
    s.key = std::move(key);
    s.label = std::move(label);
    s.type = ParamType::Int;
    s.def = def;
    s.min = min;
    s.max = max;
    s.step = step;
    return s;
}

ParamSpec ParamSpec::real(std::string key, std::string label, double def, double min, double max, double step) {
    ParamSpec s;
    s.key = std::move(key);
    s.label = std::move(label);
    s.type = ParamType::Double;
    s.def = def;
    s.min = min;
    s.max = max;
    s.step = step;
    return s;
}

ParamSpec ParamSpec::boolean(std::string key, std::string label, bool def) {
    ParamSpec s;
    s.key = std::move(key);
    s.label = std::move(label);
    s.type = ParamType::Bool;
    s.def = def ? 1.0 : 0.0;
    s.min = 0.0;
    s.max = 1.0;
    return s;
}

ParamSpec ParamSpec::choice(std::string key, std::string label, std::vector<std::string> choices, int def) {
    ParamSpec s;
    s.key = std::move(key);
    s.label = std::move(label);
    s.type = ParamType::Choice;
    s.choices = std::move(choices);
    s.def = def;
    s.min = 0.0;
    s.max = s.choices.empty() ? 0.0 : static_cast<double>(s.choices.size() - 1);
    return s;
}

ParamSpec ParamSpec::point(std::string key, std::string label, std::string linkTo) {
    ParamSpec s;
    s.key = std::move(key);
    s.label = std::move(label);
    s.type = ParamType::Point;
    s.linkTo = std::move(linkTo);
    s.min = 0.0;
    s.max = 1000000.0;
    return s;
}

ParamValues defaultValues(const std::vector<ParamSpec>& specs) {
    ParamValues values;
    for (const ParamSpec& s : specs) {
        if (s.type == ParamType::Point) {
            values[s.key + ".x"] = s.def;
            values[s.key + ".y"] = s.def;
        } else {
            values[s.key] = s.def;
        }
    }
    return values;
}

ParamValues sanitize(const std::vector<ParamSpec>& specs, const ParamValues& values) {
    ParamValues out;
    for (const ParamSpec& s : specs) {
        if (s.type == ParamType::Point) {
            for (const char* axis : {".x", ".y"}) {
                auto coordinate = values.find(s.key + axis);
                const double v = (coordinate != values.end() && std::isfinite(coordinate->second)) ? coordinate->second : s.def;
                out[s.key + axis] = std::clamp(v, s.min, s.max);
            }
            continue;
        }
        auto it = values.find(s.key);
        double v = (it != values.end() && std::isfinite(it->second)) ? it->second : s.def;
        switch (s.type) {
            case ParamType::Bool:
                v = (v != 0.0) ? 1.0 : 0.0;
                break;
            case ParamType::Int:
            case ParamType::Choice:
                v = std::round(std::clamp(v, s.min, s.max));
                break;
            case ParamType::Double:
            case ParamType::Point: // ya tratado arriba
                v = std::clamp(v, s.min, s.max);
                break;
        }
        out[s.key] = v;
    }
    return out;
}

double getReal(const ParamValues& values, const std::string& key) {
    auto it = values.find(key);
    return it != values.end() ? it->second : 0.0;
}

int getInt(const ParamValues& values, const std::string& key) {
    return static_cast<int>(std::lround(getReal(values, key)));
}

bool getBool(const ParamValues& values, const std::string& key) {
    return getReal(values, key) != 0.0;
}

std::string formatNumber(double value) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::setprecision(10) << value;
    return os.str();
}

bool parseNumber(const std::string& text, double& value) {
    std::istringstream is(text);
    is.imbue(std::locale::classic());
    double v = 0.0;
    if (!(is >> v)) return false;
    is >> std::ws;
    if (!is.eof()) return false;
    value = v;
    return true;
}

} // namespace cvcore
