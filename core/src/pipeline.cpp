/**
 * @file pipeline.cpp
 * @brief Implementación de las cadenas de comandos.
 */
#include "cvcore/pipeline.h"

#include <exception>
#include <fstream>
#include <sstream>

#include "cvcore/error.h"
#include "cvcore/image_io.h"
#include "cvcore/registry.h"

namespace cvcore {

namespace {
const char* const kHeader = "# lulumoy-chain 1";
}

void Pipeline::add(Step step) { steps_.push_back(std::move(step)); }

void Pipeline::removeLast() {
    if (!steps_.empty()) steps_.pop_back();
}

void Pipeline::clear() { steps_.clear(); }

cv::Mat Pipeline::run(const cv::Mat& input) const {
    cv::Mat image = input;
    for (const Step& step : steps_) {
        const Operation* op = Registry::instance().find(step.opId);
        if (!op) throw Error("unknown_operation", "Unknown operation", step.opId);
        cv::Mat result;
        try {
            result = op->apply(image, sanitize(op->params(), step.params));
        } catch (const Error&) {
            throw;
        } catch (const std::exception& e) {
            throw Error("operation_failed", "The operation failed", step.opId + " - " + e.what());
        }
        if (result.empty()) throw Error("operation_failed", "The operation failed", step.opId);
        image = result;
    }
    return image;
}

std::string Pipeline::toText() const {
    std::string text = kHeader;
    text += '\n';
    for (const Step& step : steps_) {
        text += step.opId;
        for (const auto& p : step.params) {
            text += ' ';
            text += p.first;
            text += '=';
            text += formatNumber(p.second);
        }
        text += '\n';
    }
    return text;
}

Pipeline Pipeline::fromText(const std::string& text) {
    Pipeline pipeline;
    std::istringstream lines(text);
    std::string line;
    int number = 0;
    while (std::getline(lines, line)) {
        ++number;
        std::istringstream tokens(line);
        std::string token;
        if (!(tokens >> token) || token[0] == '#') continue; // vacía o comentario
        // Un BOM UTF-8 al inicio del archivo no forma parte del id.
        if (token.rfind("\xEF\xBB\xBF", 0) == 0) token.erase(0, 3);
        if (token.empty() || token[0] == '#') continue;

        Step step;
        step.opId = token;
        while (tokens >> token) {
            std::size_t eq = token.find('=');
            double value = 0.0;
            if (eq == std::string::npos || eq == 0 || !parseNumber(token.substr(eq + 1), value)) {
                throw Error("bad_chain", "Invalid line in chain file", "line " + std::to_string(number));
            }
            step.params[token.substr(0, eq)] = value;
        }
        pipeline.add(std::move(step));
    }
    return pipeline;
}

void Pipeline::save(const std::string& utf8Path) const {
    std::ofstream out(fromUtf8(utf8Path), std::ios::binary | std::ios::trunc);
    if (!out) throw Error("write_failed", "Cannot write the file", utf8Path);
    out << toText();
    if (!out) throw Error("write_failed", "Cannot write the file", utf8Path);
}

Pipeline Pipeline::load(const std::string& utf8Path) {
    std::ifstream in(fromUtf8(utf8Path), std::ios::binary);
    if (!in) throw Error("read_failed", "Cannot read the file", utf8Path);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return fromText(buffer.str());
}

} // namespace cvcore
