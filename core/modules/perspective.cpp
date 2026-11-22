/**
 * @file perspective.cpp
 * @brief Módulo: transformación de perspectiva definida por cuatro pares de puntos.
 *
 * Igual que la transformación afín, los puntos se eligen con el ratón: para
 * cada par, primero el punto de origen y después dónde debe quedar. Con los
 * cuatro pares se calcula la homografía con cv::getPerspectiveTransform y se
 * aplica con cv::warpPerspective. La imagen conserva su tamaño.
 *
 * Uso típico: enderezar un documento o una fachada fotografiados en ángulo,
 * marcando sus cuatro esquinas y llevándolas a las esquinas de un rectángulo.
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

/// Doble del área del triángulo a-b-c; cerca de cero si están alineados.
double doubleArea(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
    return std::fabs(static_cast<double>(b.x - a.x) * (c.y - a.y) - static_cast<double>(c.x - a.x) * (b.y - a.y));
}

/// La homografía solo está definida si no hay tres de los cuatro puntos alineados.
bool degenerate(const cv::Point2f q[4]) {
    return doubleArea(q[0], q[1], q[2]) < 1.0 || doubleArea(q[0], q[1], q[3]) < 1.0 ||
           doubleArea(q[0], q[2], q[3]) < 1.0 || doubleArea(q[1], q[2], q[3]) < 1.0;
}

/// Transformación de perspectiva (homografía) por 4 pares de puntos.
class PerspectiveTransform final : public Operation {
public:
    std::string id() const override { return "edit.perspective"; }
    std::string menu() const override { return "edit"; }
    std::string label() const override { return "Perspective transform (4 points)"; }
    int order() const override { return 50; }
    std::vector<ParamSpec> params() const override {
        return {
            // El orden de esta lista es el orden en que se piden los clics.
            ParamSpec::point("src1", "Source point 1"),
            ParamSpec::point("dst1", "Destination of point 1", "src1"),
            ParamSpec::point("src2", "Source point 2"),
            ParamSpec::point("dst2", "Destination of point 2", "src2"),
            ParamSpec::point("src3", "Source point 3"),
            ParamSpec::point("dst3", "Destination of point 3", "src3"),
            ParamSpec::point("src4", "Source point 4"),
            ParamSpec::point("dst4", "Destination of point 4", "src4"),
            ParamSpec::choice("interpolation", "Interpolation", {"Nearest", "Linear", "Cubic"}, 1),
            ParamSpec::choice("border", "Empty areas", {"Black", "Repeat edge", "Mirror"}, 0),
        };
    }
    /// En la vista previa se marca el cuadrilátero de los puntos de origen.
    std::vector<cv::Point2f> region(const cv::Size&, const ParamValues& p) const override {
        return {point(p, "src1"), point(p, "src2"), point(p, "src3"), point(p, "src4")};
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        static const int kInterpolation[] = {cv::INTER_NEAREST, cv::INTER_LINEAR, cv::INTER_CUBIC};
        static const int kBorder[] = {cv::BORDER_CONSTANT, cv::BORDER_REPLICATE, cv::BORDER_REFLECT};

        const cv::Point2f src[4] = {point(p, "src1"), point(p, "src2"), point(p, "src3"), point(p, "src4")};
        const cv::Point2f dst[4] = {point(p, "dst1"), point(p, "dst2"), point(p, "dst3"), point(p, "dst4")};
        if (degenerate(src) || degenerate(dst)) {
            throw Error("degenerate_points4", "No three of the four points may lie on a straight line");
        }

        const cv::Mat matrix = cv::getPerspectiveTransform(src, dst);
        cv::Mat out;
        cv::warpPerspective(in, out, matrix, in.size(), kInterpolation[std::clamp(getInt(p, "interpolation"), 0, 2)],
                            kBorder[std::clamp(getInt(p, "border"), 0, 2)]);
        return out;
    }
};

CVCORE_REGISTER_OPERATION(PerspectiveTransform)

} // namespace
