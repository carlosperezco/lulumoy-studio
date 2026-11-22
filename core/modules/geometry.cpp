/**
 * @file geometry.cpp
 * @brief Módulo: rotar, voltear (flip) y recortar (crop).
 */
#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>

#include "cvcore/registry.h"

namespace {

using namespace cvcore;

/// Rota la imagen un ángulo cualquiera (positivo = antihorario).
class Rotate final : public Operation {
public:
    std::string id() const override { return "edit.rotate"; }
    std::string menu() const override { return "edit"; }
    std::string label() const override { return "Rotate"; }
    int order() const override { return 10; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::real("angle", "Angle (degrees, counter-clockwise)", 90.0, -360.0, 360.0, 1.0),
            ParamSpec::boolean("expand", "Expand canvas to fit the whole image", true),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        const double angle = getReal(p, "angle");
        const bool expand = getBool(p, "expand");

        double a = std::fmod(angle, 360.0);
        if (a < 0.0) a += 360.0;
        if (a == 0.0) return in.clone();

        cv::Mat out;
        if (expand) {
            // Los giros de 90° exactos se hacen sin interpolación (sin pérdida).
            if (a == 90.0) { cv::rotate(in, out, cv::ROTATE_90_COUNTERCLOCKWISE); return out; }
            if (a == 180.0) { cv::rotate(in, out, cv::ROTATE_180); return out; }
            if (a == 270.0) { cv::rotate(in, out, cv::ROTATE_90_CLOCKWISE); return out; }
        }

        const cv::Point2f center(in.cols / 2.0f, in.rows / 2.0f);
        cv::Mat m = cv::getRotationMatrix2D(center, angle, 1.0);
        cv::Size size = in.size();
        if (expand) {
            const cv::Rect2f box =
                cv::RotatedRect(cv::Point2f(0.0f, 0.0f),
                                cv::Size2f(static_cast<float>(in.cols), static_cast<float>(in.rows)),
                                static_cast<float>(angle))
                    .boundingRect2f();
            size = cv::Size(static_cast<int>(std::ceil(box.width)), static_cast<int>(std::ceil(box.height)));
            m.at<double>(0, 2) += size.width / 2.0 - center.x;
            m.at<double>(1, 2) += size.height / 2.0 - center.y;
        }
        cv::warpAffine(in, out, m, size, cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        return out;
    }
};

/// Voltea la imagen en horizontal, vertical o ambos.
class Flip final : public Operation {
public:
    std::string id() const override { return "edit.flip"; }
    std::string menu() const override { return "edit"; }
    std::string label() const override { return "Flip"; }
    int order() const override { return 20; }
    std::vector<ParamSpec> params() const override {
        return {ParamSpec::choice("mode", "Direction", {"Horizontal", "Vertical", "Both"}, 0)};
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        static const int kCodes[] = {1, 0, -1}; // códigos de cv::flip
        const int mode = std::clamp(getInt(p, "mode"), 0, 2);
        cv::Mat out;
        cv::flip(in, out, kCodes[mode]);
        return out;
    }
};

/// Recorta un rectángulo. Ancho o alto 0 significa "hasta el borde".
class Crop final : public Operation {
public:
    std::string id() const override { return "edit.crop"; }
    std::string menu() const override { return "edit"; }
    std::string label() const override { return "Crop"; }
    int order() const override { return 30; }
    std::vector<ParamSpec> params() const override {
        return {
            ParamSpec::integer("x", "Left (px)", 0, 0, 100000),
            ParamSpec::integer("y", "Top (px)", 0, 0, 100000),
            ParamSpec::integer("width", "Width (px, 0 = up to the edge)", 0, 0, 100000),
            ParamSpec::integer("height", "Height (px, 0 = up to the edge)", 0, 0, 100000),
        };
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        return in(rect(in.size(), p)).clone();
    }
    std::vector<cv::Point2f> region(const cv::Size& in, const ParamValues& p) const override {
        const cv::Rect r = rect(in, p);
        const float x0 = static_cast<float>(r.x), y0 = static_cast<float>(r.y);
        const float x1 = static_cast<float>(r.x + r.width), y1 = static_cast<float>(r.y + r.height);
        return {cv::Point2f(x0, y0), cv::Point2f(x1, y0), cv::Point2f(x1, y1), cv::Point2f(x0, y1)};
    }

private:
    /// Rectángulo que se conserva, ajustado a los bordes de la imagen.
    static cv::Rect rect(const cv::Size& in, const ParamValues& p) {
        const int x = std::clamp(getInt(p, "x"), 0, in.width - 1);
        const int y = std::clamp(getInt(p, "y"), 0, in.height - 1);
        int w = getInt(p, "width");
        int h = getInt(p, "height");
        if (w <= 0 || x + w > in.width) w = in.width - x;
        if (h <= 0 || y + h > in.height) h = in.height - y;
        return cv::Rect(x, y, w, h);
    }
};

CVCORE_REGISTER_OPERATION(Rotate)
CVCORE_REGISTER_OPERATION(Flip)
CVCORE_REGISTER_OPERATION(Crop)

} // namespace
