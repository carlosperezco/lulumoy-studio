/**
 * @file vectorize.cpp
 * @brief Implementación de la vectorización en blanco y negro.
 */
#include "cvcore/vectorize.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <locale>
#include <sstream>

#include <opencv2/imgproc.hpp>

#include "cvcore/error.h"
#include "cvcore/image_io.h"

namespace cvcore {

namespace {

/// Límite de la imagen de trabajo; por encima se reduce la ampliación pedida.
const double kMaxWorkPixels = 40e6;

/// Vector unitario de @p from a @p to (cero si coinciden).
cv::Point2f unit(const cv::Point2f& from, const cv::Point2f& to) {
    const cv::Point2f d = to - from;
    const float length = std::sqrt(d.x * d.x + d.y * d.y);
    return length > 0.0f ? d * (1.0f / length) : cv::Point2f(0.0f, 0.0f);
}

bool axisAligned(const cv::Point2f& d) {
    return std::fabs(d.x) < 1e-6f || std::fabs(d.y) < 1e-6f;
}

/**
 * @brief Desplaza un contorno de cv::findContours medio píxel hacia el fondo.
 *
 * findContours devuelve los centros de los píxeles del borde. OpenCV recorre
 * los contornos exteriores y los agujeros en sentidos opuestos, de modo que
 * el fondo queda siempre del mismo lado: la normal hacia el fondo de un
 * tramo con dirección d es (-d.y, d.x).
 *
 * - Giros cóncavos y esquinas de tramos horizontales/verticales: punto de
 *   inglete (intersección de las dos paralelas), exacto en esquinas de píxel.
 * - Giros convexos de hasta 90° con diagonales: bisectriz a medio píxel,
 *   para no crear picos en los escalones.
 * - Giros convexos cerrados y vueltas atrás (extremos de líneas de 1 px):
 *   dos puntos que forman un remate cuadrado.
 */
std::vector<cv::Point2f> offsetContour(const std::vector<cv::Point>& contour) {
    const float r = 0.5f;
    std::vector<cv::Point2f> out;
    const std::size_t n = contour.size();
    if (n == 0) return out;
    if (n == 1) { // un píxel aislado
        const cv::Point2f p(static_cast<float>(contour[0].x), static_cast<float>(contour[0].y));
        out = {p + cv::Point2f(-r, -r), p + cv::Point2f(r, -r), p + cv::Point2f(r, r), p + cv::Point2f(-r, r)};
        return out;
    }
    out.reserve(n + n / 4);
    for (std::size_t k = 0; k < n; ++k) {
        const cv::Point2f a(contour[(k + n - 1) % n]);
        const cv::Point2f b(contour[k]);
        const cv::Point2f c(contour[(k + 1) % n]);
        const cv::Point2f d1 = unit(a, b);
        const cv::Point2f d2 = unit(b, c);
        if ((d1.x == 0.0f && d1.y == 0.0f) || (d2.x == 0.0f && d2.y == 0.0f)) continue; // punto repetido
        const cv::Point2f n1(-d1.y, d1.x);
        const cv::Point2f n2(-d2.y, d2.x);
        const float turn = d1.dot(d2); // 1 = recto, 0 = 90°, -1 = vuelta atrás
        const bool convex = d2.dot(n1) <= 1e-6f;

        if (!convex || (axisAligned(d1) && axisAligned(d2) && turn >= -1e-6f)) {
            out.push_back(b + (n1 + n2) * (r / (1.0f + n1.dot(n2))));
        } else if (turn >= -1e-6f) {
            const cv::Point2f bisector = n1 + n2;
            const float length = std::sqrt(bisector.dot(bisector));
            out.push_back(b + bisector * (r / length));
        } else {
            out.push_back(b + (n1 + d1) * r);
            out.push_back(b + (n2 - d2) * r);
        }
    }
    return out;
}

/// Ángulo de giro en grados en el vértice @p b (0 = sigue recto, 180 = vuelve atrás).
double turnAngle(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
    const cv::Point2f v1 = b - a;
    const cv::Point2f v2 = c - b;
    const double lengths = std::sqrt(static_cast<double>(v1.dot(v1)) * v2.dot(v2));
    if (lengths <= 0.0) return 180.0;
    const double cosine = std::clamp(static_cast<double>(v1.dot(v2)) / lengths, -1.0, 1.0);
    return std::acos(cosine) * 180.0 / 3.14159265358979323846;
}

/**
 * @brief Convierte un polígono en trazado.
 *
 * Con curvas, el trazado pasa por los puntos medios de los lados y usa cada
 * vértice como punto de control de una cuadrática; los vértices que giran
 * más que @p cornerAngle se mantienen como esquinas rectas.
 */
VectorPath makePath(const std::vector<cv::Point2f>& p, bool curves, double cornerAngle) {
    VectorPath path;
    const std::size_t n = p.size();
    if (!curves) {
        path.start = p[0];
        for (std::size_t i = 1; i < n; ++i) path.segments.push_back({false, cv::Point2f(), p[i]});
        return path;
    }
    std::vector<cv::Point2f> mid(n);
    for (std::size_t i = 0; i < n; ++i) mid[i] = (p[i] + p[(i + 1) % n]) * 0.5f;
    path.start = mid[n - 1];
    for (std::size_t i = 0; i < n; ++i) {
        if (turnAngle(p[(i + n - 1) % n], p[i], p[(i + 1) % n]) > cornerAngle) {
            path.segments.push_back({false, cv::Point2f(), p[i]});
            path.segments.push_back({false, cv::Point2f(), mid[i]});
        } else {
            path.segments.push_back({true, p[i], mid[i]});
        }
    }
    return path;
}

/// Número con hasta 2 decimales, punto decimal y sin ceros sobrantes.
void writeNumber(std::ostream& os, float value) {
    const long scaled = std::lround(static_cast<double>(value) * 100.0);
    const long whole = scaled / 100;
    long frac = std::labs(scaled % 100);
    if (scaled < 0 && whole == 0) os << '-';
    os << whole;
    if (frac != 0) {
        os << '.' << static_cast<char>('0' + frac / 10);
        if (frac % 10 != 0) os << static_cast<char>('0' + frac % 10);
    }
}

void writePoint(std::ostream& os, const cv::Point2f& p) {
    writeNumber(os, p.x);
    os << ' ';
    writeNumber(os, p.y);
}

} // namespace

std::size_t VectorResult::nodes() const {
    std::size_t total = 0;
    for (const VectorPath& path : paths) total += path.segments.size();
    return total;
}

std::vector<ParamSpec> vectorizeParams() {
    return {
        ParamSpec::choice("method", "Threshold method", {"Fixed", "Automatic (Otsu)", "Adaptive"}, 1),
        ParamSpec::integer("threshold", "Threshold level (fixed method)", 128, 0, 255),
        ParamSpec::integer("block", "Block size in px (adaptive method)", 51, 3, 999, 2),
        ParamSpec::real("offset", "Offset C (adaptive method)", 10.0, -100.0, 100.0, 1.0),
        ParamSpec::real("blur", "Pre-blur (sigma, px)", 0.0, 0.0, 20.0, 0.1),
        ParamSpec::boolean("invert", "Trace light shapes (invert)", false),
        ParamSpec::choice("scale", "Working resolution", {"x1", "x2", "x4"}, 1),
        ParamSpec::integer("clean", "Edge cleaning (px)", 0, 0, 20),
        ParamSpec::real("min_area", "Ignore specks smaller than (px\xC2\xB2)", 4.0, 0.0, 1000000.0, 1.0),
        ParamSpec::real("epsilon", "Simplification (px)", 1.0, 0.0, 20.0, 0.1),
        ParamSpec::choice("curves", "Outline", {"Polygons", "Smooth curves"}, 1),
        ParamSpec::real("corner", "Keep corners sharper than (degrees)", 60.0, 0.0, 180.0, 5.0),
        ParamSpec::boolean("background", "White background in the SVG", false),
    };
}

VectorResult vectorize(const cv::Mat& image, const ParamValues& rawValues) {
    if (image.empty()) throw Error("no_image", "There is no image to save");
    const ParamValues v = sanitize(vectorizeParams(), rawValues);

    VectorResult result;
    result.width = image.cols;
    result.height = image.rows;
    result.background = getBool(v, "background");

    // 1. Grises de 8 bits y resolución de trabajo.
    cv::Mat gray = toGray(image);
    if (gray.depth() != CV_8U) {
        cv::Mat gray8;
        gray.convertTo(gray8, CV_8U, 1.0 / 257.0);
        gray = gray8;
    }
    static const int kScales[] = {1, 2, 4};
    int s = kScales[std::clamp(getInt(v, "scale"), 0, 2)];
    while (s > 1 && static_cast<double>(image.cols) * image.rows * s * s > kMaxWorkPixels) s /= 2;
    result.workScale = s;
    if (s > 1) {
        cv::Mat big;
        cv::resize(gray, big, cv::Size(), s, s, cv::INTER_CUBIC);
        gray = big;
    }

    // 2. Desenfoque y binarización (255 = forma oscura, salvo si se invierte).
    const double blur = getReal(v, "blur");
    if (blur > 0.0) cv::GaussianBlur(gray, gray, cv::Size(0, 0), blur * s);
    cv::Mat bw;
    switch (getInt(v, "method")) {
        case 0:
            cv::threshold(gray, bw, getInt(v, "threshold"), 255, cv::THRESH_BINARY_INV);
            break;
        case 2: {
            const int block = std::max(3, oddKernel(getInt(v, "block") * s));
            cv::adaptiveThreshold(gray, bw, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, block,
                                  getReal(v, "offset"));
            break;
        }
        default:
            cv::threshold(gray, bw, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
            break;
    }
    if (getBool(v, "invert")) cv::bitwise_not(bw, bw);

    // 3. Limpieza: la apertura quita salientes y manchas, el cierre tapa huecos.
    const int clean = getInt(v, "clean");
    if (clean > 0) {
        const int k = 2 * clean * s + 1;
        const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(k, k));
        cv::morphologyEx(bw, bw, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(bw, bw, cv::MORPH_CLOSE, kernel);
    }
    result.binary = bw;

    // 4. Contornos: exteriores en el primer nivel, sus agujeros como hijos.
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(bw, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_NONE);

    const double minArea = getReal(v, "min_area") * s * s;
    const double epsilon = getReal(v, "epsilon") * s;
    const bool curves = getInt(v, "curves") == 1;
    const double corner = getReal(v, "corner");
    const float inv = 1.0f / static_cast<float>(s);

    // 5 y 6. Desplazamiento, simplificación y trazado. @return false si se descarta.
    auto take = [&](int index) {
        const std::vector<cv::Point2f> edge = offsetContour(contours[static_cast<std::size_t>(index)]);
        if (edge.size() < 3 || std::fabs(cv::contourArea(edge)) < minArea) return false;
        std::vector<cv::Point2f> poly = edge;
        // Si la simplificación aplasta una forma fina (una línea), se reintenta con menos.
        for (double e = epsilon; e > 0.0; e *= 0.5) {
            cv::approxPolyDP(edge, poly, e, true);
            if (poly.size() >= 3 || e < 0.2) break;
        }
        if (poly.size() < 3) poly = edge;
        for (cv::Point2f& p : poly) p = (p + cv::Point2f(0.5f, 0.5f)) * inv; // a píxeles de la imagen original
        result.paths.push_back(makePath(poly, curves, corner));
        return true;
    };

    for (std::size_t i = 0; i < contours.size(); ++i) {
        if (hierarchy[i][3] >= 0) continue;          // los agujeros se tratan con su forma
        if (!take(static_cast<int>(i))) continue;    // forma descartada: sus agujeros también
        for (int child = hierarchy[i][2]; child >= 0; child = hierarchy[static_cast<std::size_t>(child)][0]) {
            take(child);
        }
    }
    return result;
}

std::vector<std::vector<cv::Point2f>> outlines(const VectorResult& result, int samples) {
    samples = std::max(1, samples);
    std::vector<std::vector<cv::Point2f>> out;
    out.reserve(result.paths.size());
    for (const VectorPath& path : result.paths) {
        std::vector<cv::Point2f> points{path.start};
        cv::Point2f current = path.start;
        for (const VectorSegment& seg : path.segments) {
            if (!seg.curve) {
                points.push_back(seg.end);
            } else {
                for (int j = 1; j <= samples; ++j) {
                    const float t = static_cast<float>(j) / samples;
                    const float u = 1.0f - t;
                    points.push_back(current * (u * u) + seg.control * (2.0f * u * t) + seg.end * (t * t));
                }
            }
            current = seg.end;
        }
        out.push_back(std::move(points));
    }
    return out;
}

cv::Mat renderMask(const VectorResult& result, double scale) {
    const int w = std::max(1, static_cast<int>(std::lround(result.width * scale)));
    const int h = std::max(1, static_cast<int>(std::lround(result.height * scale)));

    // fillPoly pinta también los píxeles del borde; dibujando a mayor tamaño y
    // reduciendo, ese error se reparte y los bordes quedan suavizados.
    const double pixels = static_cast<double>(w) * h;
    const int ss = pixels <= 2e6 ? 4 : pixels <= 8e6 ? 2 : 1;
    const int kShift = 3; // coordenadas con 3 bits fraccionarios
    const double k = scale * ss;

    std::vector<std::vector<cv::Point>> polys;
    for (const std::vector<cv::Point2f>& outline : outlines(result)) {
        if (outline.size() < 3) continue;
        std::vector<cv::Point> poly;
        poly.reserve(outline.size());
        for (const cv::Point2f& p : outline) {
            // Un píxel de fillPoly está centrado en coordenadas enteras; en SVG, en +0.5.
            poly.emplace_back(static_cast<int>(std::lround((p.x * k - 0.5) * (1 << kShift))),
                              static_cast<int>(std::lround((p.y * k - 0.5) * (1 << kShift))));
        }
        polys.push_back(std::move(poly));
    }

    cv::Mat big(h * ss, w * ss, CV_8UC1, cv::Scalar(0));
    if (!polys.empty()) cv::fillPoly(big, polys, cv::Scalar(255), cv::LINE_8, kShift);
    if (ss == 1) return big;
    cv::Mat mask;
    cv::resize(big, mask, cv::Size(w, h), 0, 0, cv::INTER_AREA);
    return mask;
}

std::string toSvg(const VectorResult& result) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
       << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << result.width << "\" height=\"" << result.height
       << "\" viewBox=\"0 0 " << result.width << ' ' << result.height << "\">\n"
       << "<!-- Lulumoy Studio: " << result.paths.size() << " paths, " << result.nodes() << " nodes -->\n";
    if (result.background) os << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n";
    os << "<path fill=\"#000000\" fill-rule=\"evenodd\" d=\"";
    bool first = true;
    for (const VectorPath& path : result.paths) {
        if (!first) os << '\n';
        first = false;
        os << 'M';
        writePoint(os, path.start);
        for (const VectorSegment& seg : path.segments) {
            if (seg.curve) {
                os << 'Q';
                writePoint(os, seg.control);
                os << ' ';
            } else {
                os << 'L';
            }
            writePoint(os, seg.end);
        }
        os << 'Z';
    }
    os << "\"/>\n</svg>\n";
    return os.str();
}

void saveSvgNew(const std::string& utf8Path, const std::string& svg) {
    const std::filesystem::path path = fromUtf8(utf8Path);
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        throw Error("file_exists", "The file already exists; choose a new name", utf8Path);
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) throw Error("write_failed", "Cannot write the file", utf8Path);
    out << svg;
    if (!out) throw Error("write_failed", "Cannot write the file", utf8Path);
}

} // namespace cvcore
