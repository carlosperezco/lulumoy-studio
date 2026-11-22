/**
 * @file filters_detail.cpp
 * @brief Módulo: filtros de detalle y tono (enfoque, Canny, umbral,
 *        brillo/contraste y morfología).
 */
#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "cvcore/image_io.h"
#include "cvcore/registry.h"

namespace {

using namespace cvcore;

/// Enfoque por máscara de desenfoque (unsharp mask).
class Sharpen final : public Operation {
public:
    std::string id() const override { return "filter.sharpen"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Sharpen"; }
    int order() const override { return 50; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::real("amount", "Amount", 1.0, 0.0, 5.0, 0.1),
            ParamSpec::real("sigma", "Radius (sigma)", 1.5, 0.1, 20.0, 0.1),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        const double amount = getReal(p, "amount");
        cv::Mat blurred, out;
        cv::GaussianBlur(in, blurred, cv::Size(0, 0), std::max(0.1, getReal(p, "sigma")));
        cv::addWeighted(in, 1.0 + amount, blurred, -amount, 0.0, out);
        return out;
    }
};

/// Detección de bordes de Canny (cv::Canny). El resultado es de 8 bits, 1 canal.
class Canny final : public Operation {
public:
    std::string id() const override { return "filter.canny"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Canny edges"; }
    int order() const override { return 60; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::real("threshold1", "Lower threshold", 50.0, 0.0, 1000.0, 1.0),
            ParamSpec::real("threshold2", "Upper threshold", 150.0, 0.0, 1000.0, 1.0),
            ParamSpec::choice("aperture", "Sobel aperture", {"3", "5", "7"}, 0),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        static const int kAperture[] = {3, 5, 7};
        cv::Mat gray = toGray(in);
        if (gray.depth() != CV_8U) {
            cv::Mat g8;
            gray.convertTo(g8, CV_8U, 255.0 / maxValue(gray));
            gray = g8;
        }
        cv::Mat out;
        cv::Canny(gray, out, getReal(p, "threshold1"), getReal(p, "threshold2"),
                  kAperture[std::clamp(getInt(p, "aperture"), 0, 2)]);
        return out;
    }
};

/// Umbral (cv::threshold). El nivel se da en % para que sirva en 8 y 16 bits.
class Threshold final : public Operation {
public:
    std::string id() const override { return "filter.threshold"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Threshold"; }
    int order() const override { return 70; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::real("level", "Level (%)", 50.0, 0.0, 100.0, 1.0),
            ParamSpec::choice("type", "Type",
                              {"Binary", "Binary inverted", "Truncate", "To zero", "To zero inverted"}, 0),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        static const int kTypes[] = {cv::THRESH_BINARY, cv::THRESH_BINARY_INV, cv::THRESH_TRUNC,
                                     cv::THRESH_TOZERO, cv::THRESH_TOZERO_INV};
        const double top = maxValue(in);
        cv::Mat out;
        cv::threshold(in, out, getReal(p, "level") / 100.0 * top, top,
                      kTypes[std::clamp(getInt(p, "type"), 0, 4)]);
        return out;
    }
};

/// Brillo y contraste lineales: salida = contraste * entrada + brillo.
class BrightnessContrast final : public Operation {
public:
    std::string id() const override { return "filter.brightness_contrast"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Brightness / contrast"; }
    int order() const override { return 80; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::real("brightness", "Brightness (% of range)", 0.0, -100.0, 100.0, 1.0),
            ParamSpec::real("contrast", "Contrast (factor)", 1.0, 0.0, 4.0, 0.05),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        cv::Mat out;
        in.convertTo(out, -1, getReal(p, "contrast"), getReal(p, "brightness") / 100.0 * maxValue(in));
        return out;
    }
};

/// Operaciones morfológicas (cv::morphologyEx).
class Morphology final : public Operation {
public:
    std::string id() const override { return "filter.morphology"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Morphology"; }
    int order() const override { return 90; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::choice("operation", "Operation", {"Erode", "Dilate", "Open", "Close", "Gradient"}, 0),
            ParamSpec::choice("shape", "Kernel shape", {"Rectangle", "Ellipse", "Cross"}, 0),
            ParamSpec::integer("ksize", "Kernel size", 3, 1, 99, 1),
            ParamSpec::integer("iterations", "Iterations", 1, 1, 20, 1),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        static const int kOps[] = {cv::MORPH_ERODE, cv::MORPH_DILATE, cv::MORPH_OPEN, cv::MORPH_CLOSE,
                                   cv::MORPH_GRADIENT};
        static const int kShapes[] = {cv::MORPH_RECT, cv::MORPH_ELLIPSE, cv::MORPH_CROSS};
        const int k = std::max(1, getInt(p, "ksize"));
        const cv::Mat kernel =
            cv::getStructuringElement(kShapes[std::clamp(getInt(p, "shape"), 0, 2)], cv::Size(k, k));
        cv::Mat out;
        cv::morphologyEx(in, out, kOps[std::clamp(getInt(p, "operation"), 0, 4)], kernel, cv::Point(-1, -1),
                         std::max(1, getInt(p, "iterations")));
        return out;
    }
};

CVCORE_REGISTER_OPERATION(Sharpen)
CVCORE_REGISTER_OPERATION(Canny)
CVCORE_REGISTER_OPERATION(Threshold)
CVCORE_REGISTER_OPERATION(BrightnessContrast)
CVCORE_REGISTER_OPERATION(Morphology)

} // namespace
