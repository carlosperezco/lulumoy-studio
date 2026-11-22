/**
 * @file image_io.cpp
 * @brief Implementación de lectura/escritura y utilidades de imagen.
 *
 * Los archivos se leen y escriben con flujos estándar y se codifican en
 * memoria (cv::imdecode / cv::imencode). Así las rutas con acentos
 * funcionan igual en Windows y en Linux.
 */
#include "cvcore/image_io.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "cvcore/error.h"

namespace cvcore {

namespace fs = std::filesystem;

fs::path fromUtf8(const std::string& utf8Path) {
    return fs::u8path(utf8Path);
}

std::string toUtf8(const fs::path& path) {
    const auto s = path.u8string(); // std::string en C++17, std::u8string en C++20
    return std::string(s.begin(), s.end());
}

namespace {

std::string lowerExtension(const fs::path& path) {
    std::string ext = toUtf8(path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

bool supports16Bits(const std::string& ext) {
    static const char* const kExt[] = {".png", ".tif", ".tiff", ".jp2", ".pgm", ".ppm", ".pnm"};
    return std::any_of(std::begin(kExt), std::end(kExt), [&](const char* e) { return ext == e; });
}

} // namespace

cv::Mat loadImage(const std::string& utf8Path) {
    std::ifstream in(fromUtf8(utf8Path), std::ios::binary);
    if (!in) throw Error("read_failed", "Cannot read the file", utf8Path);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (bytes.empty()) throw Error("read_failed", "Cannot read the file", utf8Path);

    cv::Mat image;
    try {
        image = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
    } catch (const cv::Exception&) {
        image.release();
    }
    if (image.empty()) throw Error("bad_format", "The file is not a supported image", utf8Path);

    // Normaliza a los tipos con los que trabaja la aplicación.
    if (image.channels() == 2) {
        cv::Mat first;
        cv::extractChannel(image, first, 0);
        image = first;
    }
    if (image.depth() != CV_8U && image.depth() != CV_16U) {
        cv::Mat scaled;
        cv::normalize(image, scaled, 0, 65535, cv::NORM_MINMAX, CV_16U);
        image = scaled;
    }
    return image;
}

void saveImageNew(const std::string& utf8Path, const cv::Mat& image) {
    if (image.empty()) throw Error("no_image", "There is no image to save");
    const fs::path path = fromUtf8(utf8Path);
    std::error_code ec;
    if (fs::exists(path, ec)) throw Error("file_exists", "The file already exists; choose a new name", utf8Path);

    const std::string ext = lowerExtension(path);
    if (ext.empty()) throw Error("bad_format", "The file name needs an extension (.png, .jpg, .tif...)", utf8Path);
    if (image.depth() == CV_16U && !supports16Bits(ext)) {
        throw Error("needs_16bit_format", "This format cannot store 16-bit images; use PNG or TIFF", ext);
    }

    std::vector<unsigned char> bytes;
    bool ok = false;
    try {
        ok = cv::imencode(ext, image, bytes);
    } catch (const cv::Exception&) {
        ok = false;
    }
    if (!ok) throw Error("bad_format", "Cannot encode the image in this format", ext);

    std::ofstream out(path, std::ios::binary);
    if (!out) throw Error("write_failed", "Cannot write the file", utf8Path);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw Error("write_failed", "Cannot write the file", utf8Path);
}

std::string suggestNewPath(const std::string& utf8Path) {
    const fs::path original = fromUtf8(utf8Path);
    const fs::path dir = original.parent_path();
    const std::string stem = toUtf8(original.stem());
    std::string ext = toUtf8(original.extension());
    if (ext.empty()) ext = ".png";

    std::error_code ec;
    for (int i = 1; i < 10000; ++i) {
        const std::string name = stem + "_edit" + (i == 1 ? "" : "_" + std::to_string(i)) + ext;
        const fs::path candidate = dir / fromUtf8(name);
        if (!fs::exists(candidate, ec)) return toUtf8(candidate);
    }
    return toUtf8(dir / fromUtf8(stem + "_edit_new" + ext));
}

cv::Mat toDisplayRgb(const cv::Mat& image) {
    if (image.empty()) return cv::Mat();
    cv::Mat m = image;
    if (m.depth() != CV_8U) {
        cv::Mat m8;
        m.convertTo(m8, CV_8U, 255.0 / maxValue(m));
        m = m8;
    }
    cv::Mat rgb;
    switch (m.channels()) {
        case 1: cv::cvtColor(m, rgb, cv::COLOR_GRAY2RGB); break;
        case 4: cv::cvtColor(m, rgb, cv::COLOR_BGRA2RGB); break;
        default: cv::cvtColor(m, rgb, cv::COLOR_BGR2RGB); break;
    }
    return rgb;
}

cv::Mat toGray(const cv::Mat& image) {
    cv::Mat gray;
    switch (image.channels()) {
        case 3: cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY); break;
        case 4: cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY); break;
        default: gray = image.clone(); break;
    }
    return gray;
}

double maxValue(const cv::Mat& image) {
    return image.depth() == CV_16U ? 65535.0 : 255.0;
}

int oddKernel(int size) {
    if (size < 1) size = 1;
    return (size % 2 == 0) ? size + 1 : size;
}

} // namespace cvcore
