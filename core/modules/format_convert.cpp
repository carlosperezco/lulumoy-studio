/**
 * @file format_convert.cpp
 * @brief Módulo: cambio de formato (8 bits, 16 bits, grises, color).
 */
#include <opencv2/imgproc.hpp>

#include "cvcore/image_io.h"
#include "cvcore/registry.h"

namespace {

using namespace cvcore;

/// Convierte la imagen a 8 bits por canal.
class To8Bit final : public Operation {
public:
    std::string id() const override { return "format.to8bit"; }
    std::string menu() const override { return "edit/format"; }
    std::string label() const override { return "8 bits"; }
    int order() const override { return 10; }
    cv::Mat apply(const cv::Mat& in, const ParamValues&) const override {
        if (in.depth() == CV_8U) return in.clone();
        cv::Mat out;
        in.convertTo(out, CV_8U, 1.0 / 257.0); // 65535 -> 255
        return out;
    }
};

/// Convierte la imagen a 16 bits por canal.
class To16Bit final : public Operation {
public:
    std::string id() const override { return "format.to16bit"; }
    std::string menu() const override { return "edit/format"; }
    std::string label() const override { return "16 bits"; }
    int order() const override { return 20; }
    cv::Mat apply(const cv::Mat& in, const ParamValues&) const override {
        if (in.depth() == CV_16U) return in.clone();
        cv::Mat out;
        in.convertTo(out, CV_16U, 257.0); // 255 -> 65535
        return out;
    }
};

/// Convierte la imagen a escala de grises (1 canal).
class ToGray final : public Operation {
public:
    std::string id() const override { return "format.togray"; }
    std::string menu() const override { return "edit/format"; }
    std::string label() const override { return "Grayscale"; }
    int order() const override { return 30; }
    cv::Mat apply(const cv::Mat& in, const ParamValues&) const override { return toGray(in); }
};

/// Convierte la imagen a color (3 canales BGR).
class ToColor final : public Operation {
public:
    std::string id() const override { return "format.tocolor"; }
    std::string menu() const override { return "edit/format"; }
    std::string label() const override { return "Color"; }
    int order() const override { return 40; }
    cv::Mat apply(const cv::Mat& in, const ParamValues&) const override {
        cv::Mat out;
        switch (in.channels()) {
            case 1: cv::cvtColor(in, out, cv::COLOR_GRAY2BGR); break;
            case 4: cv::cvtColor(in, out, cv::COLOR_BGRA2BGR); break;
            default: out = in.clone(); break;
        }
        return out;
    }
};

CVCORE_REGISTER_OPERATION(To8Bit)
CVCORE_REGISTER_OPERATION(To16Bit)
CVCORE_REGISTER_OPERATION(ToGray)
CVCORE_REGISTER_OPERATION(ToColor)

} // namespace
