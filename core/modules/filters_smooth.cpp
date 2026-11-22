/**
 * @file filters_smooth.cpp
 * @brief Módulo: filtros de suavizado de OpenCV (gaussiano, media, mediana, bilateral).
 */
#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "cvcore/error.h"
#include "cvcore/image_io.h"
#include "cvcore/registry.h"

namespace {

using namespace cvcore;

/// Desenfoque gaussiano (cv::GaussianBlur).
class GaussianBlur final : public Operation {
public:
    std::string id() const override { return "filter.gaussian"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Gaussian blur"; }
    int order() const override { return 10; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::integer("ksize", "Kernel size (odd)", 5, 1, 99, 2),
            ParamSpec::real("sigma", "Sigma (0 = automatic)", 0.0, 0.0, 50.0, 0.1),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        const int k = oddKernel(getInt(p, "ksize"));
        cv::Mat out;
        cv::GaussianBlur(in, out, cv::Size(k, k), getReal(p, "sigma"));
        return out;
    }
};

/// Desenfoque de media (cv::blur).
class BoxBlur final : public Operation {
public:
    std::string id() const override { return "filter.box"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Box blur"; }
    int order() const override { return 20; }
    std::vector<ParamSpec> params() const override {
        return {ParamSpec::integer("ksize", "Kernel size", 5, 1, 99, 1)};
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        const int k = std::max(1, getInt(p, "ksize"));
        cv::Mat out;
        cv::blur(in, out, cv::Size(k, k));
        return out;
    }
};

/// Filtro de mediana (cv::medianBlur).
class MedianBlur final : public Operation {
public:
    std::string id() const override { return "filter.median"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Median blur"; }
    int order() const override { return 30; }
    std::vector<ParamSpec> params() const override {
        return {ParamSpec::integer("ksize", "Kernel size (odd)", 5, 3, 99, 2)};
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        const int k = std::max(3, oddKernel(getInt(p, "ksize")));
        // OpenCV solo admite núcleos mayores de 5 en imágenes de 8 bits.
        if (k > 5 && in.depth() != CV_8U) {
            throw Error("needs_8bit", "This operation needs an 8-bit image", "median, ksize > 5");
        }
        cv::Mat out;
        cv::medianBlur(in, out, k);
        return out;
    }
};

/// Filtro bilateral (cv::bilateralFilter): suaviza conservando bordes.
class Bilateral final : public Operation {
public:
    std::string id() const override { return "filter.bilateral"; }
    std::string menu() const override { return "edit/filters"; }
    std::string label() const override { return "Bilateral filter"; }
    int order() const override { return 40; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::integer("d", "Neighbourhood diameter", 9, 1, 25, 2),
            ParamSpec::real("sigma_color", "Sigma colour", 75.0, 1.0, 255.0, 1.0),
            ParamSpec::real("sigma_space", "Sigma space", 75.0, 1.0, 255.0, 1.0),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        if (in.depth() != CV_8U) throw Error("needs_8bit", "This operation needs an 8-bit image", "bilateral");
        if (in.channels() == 4) throw Error("needs_no_alpha", "This operation needs a grayscale or colour image without alpha", "bilateral");
        cv::Mat out;
        cv::bilateralFilter(in, out, getInt(p, "d"), getReal(p, "sigma_color"), getReal(p, "sigma_space"));
        return out;
    }
};

CVCORE_REGISTER_OPERATION(GaussianBlur)
CVCORE_REGISTER_OPERATION(BoxBlur)
CVCORE_REGISTER_OPERATION(MedianBlur)
CVCORE_REGISTER_OPERATION(Bilateral)

} // namespace
