/**
 * @file affine.cpp
 * @brief Módulo: transformación afín definida por tres pares de puntos.
 *
 * Los puntos se eligen con el ratón sobre la imagen: para cada par se marca
 * primero el punto de origen y después dónde debe quedar (destino). Con los
 * tres pares se calcula la matriz con cv::getAffineTransform y se aplica con
 * cv::warpAffine. La imagen resultante conserva el tamaño de la original.
 */
#include <algorithm>
#include <cmath>
#include <string>

#include <opencv2/imgproc.hpp>

#include "cvcore/error.h"
#include "cvcore/registry.h"

namespace {

using namespace cvcore;

/// Lee un parámetro de tipo punto.
cv::Point2f point(const ParamValues& p, const std::string& key) {
    return cv::Point2f(static_cast<float>(getReal(p, key + ".x")), static_cast<float>(getReal(p, key + ".y")));
}

/// Doble del área del triángulo; cerca de cero si los puntos están alineados.
double doubleArea(const cv::Point2f t[3]) {
    return std::fabs(static_cast<double>(t[1].x - t[0].x) * (t[2].y - t[0].y) -
                     static_cast<double>(t[2].x - t[0].x) * (t[1].y - t[0].y));
}

/// Transformación afín (traslación, giro, escala e inclinación) por 3 pares de puntos.
class AffineTransform final : public Operation {
public:
    std::string id() const override { return "edit.affine"; }
    std::string menu() const override { return "edit"; }
    std::string label() const override { return "Affine transform (3 points)"; }
    int order() const override { return 40; }
    std::vector<ParamSpec> params() const override {
        return {
            // El orden de esta lista es el orden en que se piden los clics.
            ParamSpec::point("src1", "Source point 1"),
            ParamSpec::point("dst1", "Destination of point 1", "src1"),
            ParamSpec::point("src2", "Source point 2"),
            ParamSpec::point("dst2", "Destination of point 2", "src2"),
            ParamSpec::point("src3", "Source point 3"),
            ParamSpec::point("dst3", "Destination of point 3", "src3"),
            ParamSpec::choice("interpolation", "Interpolation", {"Nearest", "Linear", "Cubic"}, 1),
            ParamSpec::choice("border", "Empty areas", {"Black", "Repeat edge", "Mirror"}, 0),
        };
    }
    /// En la vista previa se marca el triángulo de los puntos de origen.
    std::vector<cv::Point2f> region(const cv::Size&, const ParamValues& p) const override {
        return {point(p, "src1"), point(p, "src2"), point(p, "src3")};
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        static const int kInterpolation[] = {cv::INTER_NEAREST, cv::INTER_LINEAR, cv::INTER_CUBIC};
        static const int kBorder[] = {cv::BORDER_CONSTANT, cv::BORDER_REPLICATE, cv::BORDER_REFLECT};

        const cv::Point2f src[3] = {point(p, "src1"), point(p, "src2"), point(p, "src3")};
        const cv::Point2f dst[3] = {point(p, "dst1"), point(p, "dst2"), point(p, "dst3")};
        if (doubleArea(src) < 1.0 || doubleArea(dst) < 1.0) {
            throw Error("degenerate_points", "The three points must not lie on a straight line");
        }

        const cv::Mat matrix = cv::getAffineTransform(src, dst);
        cv::Mat out;
        cv::warpAffine(in, out, matrix, in.size(), kInterpolation[std::clamp(getInt(p, "interpolation"), 0, 2)],
                       kBorder[std::clamp(getInt(p, "border"), 0, 2)]);
        return out;
    }
};

CVCORE_REGISTER_OPERATION(AffineTransform)

} // namespace
