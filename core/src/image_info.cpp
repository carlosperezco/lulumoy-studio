/**
 * @file image_info.cpp
 * @brief Lectura de propiedades y metadatos de imagen sin librerías externas.
 *
 * Contiene lectores mínimos de las cabeceras de JPEG, PNG, TIFF, BMP, GIF y
 * WebP y un intérprete de EXIF (que es una estructura TIFF incrustada).
 * Todas las lecturas comprueban los límites del archivo, de modo que un
 * archivo dañado no puede provocar accesos fuera de memoria.
 */
#include "cvcore/image_info.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <vector>

#include "cvcore/image_io.h"

namespace cvcore {

namespace {

namespace fs = std::filesystem;
using Bytes = std::vector<unsigned char>;

const char* const kTimes = " \xC3\x97 "; // " × "

// ---------------------------------------------------------------------------
// Utilidades de texto
// ---------------------------------------------------------------------------

/// Número con decimales fijos y punto decimal, sin ceros finales.
std::string fixed(double value, int decimals) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << std::fixed << std::setprecision(decimals) << value;
    std::string s = os.str();
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}

std::string bytesText(std::uintmax_t bytes) {
    std::string text = std::to_string(bytes) + " bytes";
    const double b = static_cast<double>(bytes);
    if (b >= 1024.0 * 1024.0 * 1024.0) text += " (" + fixed(b / (1024.0 * 1024.0 * 1024.0), 2) + " GB)";
    else if (b >= 1024.0 * 1024.0) text += " (" + fixed(b / (1024.0 * 1024.0), 2) + " MB)";
    else if (b >= 1024.0) text += " (" + fixed(b / 1024.0, 1) + " kB)";
    return text;
}

bool isValidUtf8(const std::string& s) {
    std::size_t i = 0;
    while (i < s.size()) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        std::size_t extra = 0;
        if (c < 0x80) extra = 0;
        else if (c >= 0xC2 && c <= 0xDF) extra = 1;
        else if (c >= 0xE0 && c <= 0xEF) extra = 2;
        else if (c >= 0xF0 && c <= 0xF4) extra = 3;
        else return false;
        if (i + extra >= s.size() + (extra == 0 ? 1 : 0)) return false;
        for (std::size_t k = 1; k <= extra; ++k) {
            if ((static_cast<unsigned char>(s[i + k]) & 0xC0) != 0x80) return false;
        }
        i += extra + 1;
    }
    return true;
}

std::string latin1ToUtf8(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (const char ch : s) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (c < 0x80) {
            out += static_cast<char>(c);
        } else {
            out += static_cast<char>(0xC0 | (c >> 6));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return out;
}

/// Deja un texto de metadatos listo para mostrar: UTF-8 válido, sin
/// caracteres de control, sin espacios en los extremos y de longitud limitada.
std::string cleanText(std::string s, bool latin1 = false) {
    const std::size_t nul = s.find('\0');
    if (nul != std::string::npos) s.erase(nul);
    if (latin1 || !isValidUtf8(s)) s = latin1ToUtf8(s);
    for (char& c : s) {
        if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F) c = ' ';
    }
    const std::size_t first = s.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    s = s.substr(first, s.find_last_not_of(' ') - first + 1);
    const std::size_t kMax = 400;
    if (s.size() > kMax) {
        std::size_t cut = kMax;
        while (cut > 0 && (static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80) --cut; // no partir un carácter
        s = s.substr(0, cut) + "...";
    }
    return s;
}

/// Lista de propiedades en construcción; ignora los valores vacíos.
class Items {
public:
    explicit Items(std::vector<InfoItem>& out) : out_(out) {}
    void add(const char* group, const std::string& key, const std::string& label, const std::string& value) {
        if (!value.empty()) out_.push_back({group, key, label, value});
    }

private:
    std::vector<InfoItem>& out_;
};

// ---------------------------------------------------------------------------
// Lectura binaria con comprobación de límites
// ---------------------------------------------------------------------------

bool has(const Bytes& d, std::size_t off, std::size_t n) {
    return off <= d.size() && n <= d.size() - off;
}
unsigned u8(const Bytes& d, std::size_t off) {
    return has(d, off, 1) ? d[off] : 0u;
}
unsigned u16(const Bytes& d, std::size_t off, bool le) {
    if (!has(d, off, 2)) return 0u;
    return le ? (d[off] | (d[off + 1] << 8)) : ((d[off] << 8) | d[off + 1]);
}
std::uint32_t u32(const Bytes& d, std::size_t off, bool le) {
    if (!has(d, off, 4)) return 0u;
    const std::uint32_t a = d[off], b = d[off + 1], c = d[off + 2], e = d[off + 3];
    return le ? (a | (b << 8) | (c << 16) | (e << 24)) : ((a << 24) | (b << 16) | (c << 8) | e);
}
std::string str(const Bytes& d, std::size_t off, std::size_t n) {
    if (!has(d, off, n)) return "";
    return std::string(reinterpret_cast<const char*>(d.data()) + off, n);
}
bool startsWith(const Bytes& d, std::size_t off, const char* text) {
    const std::size_t n = std::strlen(text);
    return has(d, off, n) && std::memcmp(d.data() + off, text, n) == 0;
}

std::string dpiText(double x, double y) {
    if (x <= 0.0 || y <= 0.0) return "";
    if (std::fabs(x - y) < 0.01) return fixed(x, 1) + " dpi";
    return fixed(x, 1) + kTimes + fixed(y, 1) + " dpi";
}

// ---------------------------------------------------------------------------
// TIFF / EXIF
// ---------------------------------------------------------------------------

/// Una entrada de un directorio TIFF (IFD).
struct Entry {
    unsigned tag = 0;
    unsigned type = 0;
    std::uint32_t count = 0;
    std::size_t offset = 0; ///< Posición absoluta de los datos dentro del archivo.
};

/// Bloque TIFF dentro de un archivo: empieza en @c base y termina en @c end.
struct Tiff {
    const Bytes& d;
    std::size_t base = 0;
    std::size_t end = 0;
    bool le = true;

    bool ok(std::size_t off, std::size_t n) const { return off >= base && off <= end && n <= end - off; }
};

std::size_t typeSize(unsigned type) {
    static const std::size_t kSize[13] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};
    return type < 13 ? kSize[type] : 0;
}

/// Lee las entradas de un IFD. @return posición relativa del siguiente IFD (0 si no hay).
std::uint32_t readIfd(const Tiff& t, std::uint32_t ifdOffset, std::vector<Entry>& entries) {
    const std::size_t start = t.base + ifdOffset;
    if (ifdOffset == 0 || !t.ok(start, 2)) return 0;
    const unsigned count = u16(t.d, start, t.le);
    if (count > 1000 || !t.ok(start + 2, static_cast<std::size_t>(count) * 12)) return 0;

    for (unsigned i = 0; i < count; ++i) {
        const std::size_t p = start + 2 + static_cast<std::size_t>(i) * 12;
        Entry e;
        e.tag = u16(t.d, p, t.le);
        e.type = u16(t.d, p + 2, t.le);
        e.count = u32(t.d, p + 4, t.le);
        const std::size_t unit = typeSize(e.type);
        if (unit == 0 || e.count == 0 || e.count > 1000000) continue;
        const std::size_t size = unit * e.count;
        e.offset = size <= 4 ? p + 8 : t.base + u32(t.d, p + 8, t.le);
        if (!t.ok(e.offset, size)) continue;
        entries.push_back(e);
    }
    const std::size_t next = start + 2 + static_cast<std::size_t>(count) * 12;
    return t.ok(next, 4) ? u32(t.d, next, t.le) : 0;
}

/// Valor numérico número @p index de una entrada (los racionales se dividen).
double number(const Tiff& t, const Entry& e, std::uint32_t index = 0) {
    if (index >= e.count) return 0.0;
    const std::size_t p = e.offset + index * typeSize(e.type);
    switch (e.type) {
        case 1: case 7: return u8(t.d, p);
        case 6: return static_cast<std::int8_t>(u8(t.d, p));
        case 3: return u16(t.d, p, t.le);
        case 8: return static_cast<std::int16_t>(u16(t.d, p, t.le));
        case 4: return u32(t.d, p, t.le);
        case 9: return static_cast<std::int32_t>(u32(t.d, p, t.le));
        case 5: {
            const double den = u32(t.d, p + 4, t.le);
            return den != 0.0 ? u32(t.d, p, t.le) / den : 0.0;
        }
        case 10: {
            const double den = static_cast<std::int32_t>(u32(t.d, p + 4, t.le));
            return den != 0.0 ? static_cast<std::int32_t>(u32(t.d, p, t.le)) / den : 0.0;
        }
        default: return 0.0;
    }
}

std::string text(const Tiff& t, const Entry& e) {
    if (e.type != 2) return "";
    return cleanText(str(t.d, e.offset, std::min<std::size_t>(e.count, 2000)));
}

const Entry* find(const std::vector<Entry>& entries, unsigned tag) {
    for (const Entry& e : entries) {
        if (e.tag == tag) return &e;
    }
    return nullptr;
}

std::string orientationText(int v) {
    static const char* const kText[9] = {"", "1 (normal)", "2 (mirrored horizontally)", "3 (rotated 180\xC2\xB0)",
                                         "4 (mirrored vertically)", "5 (mirrored + rotated 270\xC2\xB0)",
                                         "6 (rotated 90\xC2\xB0)", "7 (mirrored + rotated 90\xC2\xB0)",
                                         "8 (rotated 270\xC2\xB0)"};
    return (v >= 1 && v <= 8) ? kText[v] : std::to_string(v);
}

std::string compressionText(int v) {
    switch (v) {
        case 1: return "None";
        case 2: case 3: case 4: return "CCITT";
        case 5: return "LZW";
        case 6: case 7: return "JPEG";
        case 8: case 32946: return "Deflate (ZIP)";
        case 32773: return "PackBits";
        case 34712: return "JPEG 2000";
        case 50000: return "Zstandard";
        case 50001: return "WebP";
        default: return std::to_string(v);
    }
}

std::string photometricText(int v) {
    switch (v) {
        case 0: return "Grayscale (white is zero)";
        case 1: return "Grayscale";
        case 2: return "RGB";
        case 3: return "Palette";
        case 5: return "CMYK";
        case 6: return "YCbCr";
        case 8: return "CIE L*a*b*";
        case 32803: return "Bayer (CFA)";
        default: return std::to_string(v);
    }
}

std::string exposureProgramText(int v) {
    static const char* const kText[9] = {"Not defined", "Manual", "Automatic", "Aperture priority", "Shutter priority",
                                         "Creative", "Action", "Portrait", "Landscape"};
    return (v >= 0 && v <= 8) ? kText[v] : std::to_string(v);
}

std::string meteringText(int v) {
    static const char* const kText[7] = {"Unknown", "Average", "Center-weighted", "Spot", "Multi-spot", "Pattern",
                                         "Partial"};
    return (v >= 0 && v <= 6) ? kText[v] : std::to_string(v);
}

/// Coordenada GPS (grados, minutos, segundos) en grados decimales.
std::string gpsCoordinate(const Tiff& t, const Entry* value, const Entry* ref) {
    if (!value || value->count < 3) return "";
    const double degrees = number(t, *value, 0) + number(t, *value, 1) / 60.0 + number(t, *value, 2) / 3600.0;
    std::string result = fixed(degrees, 6) + "\xC2\xB0";
    if (ref) result += " " + text(t, *ref);
    return result;
}

/// Interpreta un bloque TIFF. Sirve para archivos TIFF (@p isFile) y para EXIF incrustado.
void parseTiff(const Bytes& d, std::size_t base, std::size_t end, bool isFile, Items& items) {
    if (end > d.size()) end = d.size();
    if (base >= end || end - base < 8) return;
    Tiff t{d, base, end, true};
    if (startsWith(d, base, "II")) t.le = true;
    else if (startsWith(d, base, "MM")) t.le = false;
    else return;
    if (u16(d, base + 2, t.le) != 42) return;

    std::vector<Entry> ifd0;
    std::uint32_t next = readIfd(t, u32(d, base + 4, t.le), ifd0);
    const Entry* e = nullptr;

    if (isFile) {
        const Entry* w = find(ifd0, 0x0100);
        const Entry* h = find(ifd0, 0x0101);
        if (w && h) {
            items.add("format", "format.dimensions", "Dimensions",
                      fixed(number(t, *w), 0) + kTimes + fixed(number(t, *h), 0) + " px");
        }
        if ((e = find(ifd0, 0x0102))) items.add("format", "format.bit_depth", "Bits per channel", fixed(number(t, *e), 0));
        if ((e = find(ifd0, 0x0115))) items.add("format", "format.channels", "Channels", fixed(number(t, *e), 0));
        if ((e = find(ifd0, 0x0106))) items.add("format", "format.color", "Colour type", photometricText(static_cast<int>(number(t, *e))));
        if ((e = find(ifd0, 0x0103))) items.add("format", "format.compression", "Compression", compressionText(static_cast<int>(number(t, *e))));
        int pages = 1;
        std::vector<Entry> other;
        while (next != 0 && pages < 10000) {
            other.clear();
            next = readIfd(t, next, other);
            if (other.empty()) break;
            ++pages;
        }
        if (pages > 1) items.add("format", "format.pages", "Pages", std::to_string(pages));
    }

    // Resolución (se guarda en IFD0 tanto en TIFF como en EXIF).
    const Entry* xr = find(ifd0, 0x011A);
    const Entry* yr = find(ifd0, 0x011B);
    if (xr && yr) {
        const Entry* unit = find(ifd0, 0x0128);
        const double factor = (unit && number(t, *unit) == 3.0) ? 2.54 : 1.0; // 3 = píxeles por cm
        items.add(isFile ? "format" : "exif", isFile ? "format.resolution" : "exif.resolution", "Resolution",
                  dpiText(number(t, *xr) * factor, number(t, *yr) * factor));
    }

    if ((e = find(ifd0, 0x010F))) items.add("exif", "exif.make", "Camera make", text(t, *e));
    if ((e = find(ifd0, 0x0110))) items.add("exif", "exif.model", "Camera model", text(t, *e));
    if ((e = find(ifd0, 0x0131))) items.add("exif", "exif.software", "Software", text(t, *e));
    if ((e = find(ifd0, 0x0132))) items.add("exif", "exif.datetime", "Date modified (metadata)", text(t, *e));
    if ((e = find(ifd0, 0x0112))) items.add("exif", "exif.orientation", "Orientation", orientationText(static_cast<int>(number(t, *e))));
    if ((e = find(ifd0, 0x010E))) items.add("exif", "exif.description", "Description", text(t, *e));
    if ((e = find(ifd0, 0x013B))) items.add("exif", "exif.artist", "Author", text(t, *e));
    if ((e = find(ifd0, 0x8298))) items.add("exif", "exif.copyright", "Copyright", text(t, *e));

    // Sub-directorio EXIF: datos de la toma.
    if ((e = find(ifd0, 0x8769))) {
        std::vector<Entry> exif;
        readIfd(t, static_cast<std::uint32_t>(number(t, *e)), exif);
        if ((e = find(exif, 0x9003))) items.add("exif", "exif.date_original", "Date taken", text(t, *e));
        if ((e = find(exif, 0x829A))) {
            const double s = number(t, *e);
            if (s > 0.0) items.add("exif", "exif.exposure_time", "Exposure time", (s < 1.0 ? "1/" + fixed(1.0 / s, 0) : fixed(s, 1)) + " s");
        }
        if ((e = find(exif, 0x829D))) items.add("exif", "exif.fnumber", "Aperture", "f/" + fixed(number(t, *e), 1));
        if ((e = find(exif, 0x8827))) items.add("exif", "exif.iso", "ISO", fixed(number(t, *e), 0));
        if ((e = find(exif, 0x920A))) items.add("exif", "exif.focal_length", "Focal length", fixed(number(t, *e), 1) + " mm");
        if ((e = find(exif, 0xA405))) items.add("exif", "exif.focal_length_35", "Focal length (35 mm equivalent)", fixed(number(t, *e), 0) + " mm");
        if ((e = find(exif, 0x9204))) items.add("exif", "exif.exposure_bias", "Exposure compensation", fixed(number(t, *e), 2) + " EV");
        if ((e = find(exif, 0x8822))) items.add("exif", "exif.exposure_program", "Exposure program", exposureProgramText(static_cast<int>(number(t, *e))));
        if ((e = find(exif, 0x9207))) items.add("exif", "exif.metering", "Metering mode", meteringText(static_cast<int>(number(t, *e))));
        if ((e = find(exif, 0x9209))) items.add("exif", "exif.flash", "Flash", (static_cast<int>(number(t, *e)) & 1) ? "Fired" : "Not fired");
        if ((e = find(exif, 0xA403))) items.add("exif", "exif.white_balance", "White balance", number(t, *e) == 0.0 ? "Automatic" : "Manual");
        if ((e = find(exif, 0xA001))) {
            const int v = static_cast<int>(number(t, *e));
            items.add("exif", "exif.color_space", "Colour space", v == 1 ? "sRGB" : v == 65535 ? "Uncalibrated" : std::to_string(v));
        }
        const Entry* pw = find(exif, 0xA002);
        const Entry* ph = find(exif, 0xA003);
        if (pw && ph) items.add("exif", "exif.pixel_dimensions", "Original dimensions", fixed(number(t, *pw), 0) + kTimes + fixed(number(t, *ph), 0) + " px");
        if ((e = find(exif, 0xA433))) items.add("exif", "exif.lens_make", "Lens make", text(t, *e));
        if ((e = find(exif, 0xA434))) items.add("exif", "exif.lens_model", "Lens model", text(t, *e));
        if ((e = find(exif, 0xA430))) items.add("exif", "exif.owner", "Camera owner", text(t, *e));
        if ((e = find(exif, 0xA431))) items.add("exif", "exif.serial", "Camera serial number", text(t, *e));
    }

    // Sub-directorio GPS.
    if ((e = find(ifd0, 0x8825))) {
        std::vector<Entry> gps;
        readIfd(t, static_cast<std::uint32_t>(number(t, *e)), gps);
        items.add("gps", "gps.latitude", "Latitude", gpsCoordinate(t, find(gps, 2), find(gps, 1)));
        items.add("gps", "gps.longitude", "Longitude", gpsCoordinate(t, find(gps, 4), find(gps, 3)));
        if ((e = find(gps, 6))) {
            const Entry* ref = find(gps, 5);
            const double sign = (ref && number(t, *ref) == 1.0) ? -1.0 : 1.0;
            items.add("gps", "gps.altitude", "Altitude", fixed(sign * number(t, *e), 1) + " m");
        }
        if ((e = find(gps, 29))) items.add("gps", "gps.date", "Date (UTC)", text(t, *e));
    }
}

// ---------------------------------------------------------------------------
// JPEG
// ---------------------------------------------------------------------------

void parseJpeg(const Bytes& d, Items& items) {
    std::size_t iccBytes = 0;
    bool xmp = false, iptc = false;
    std::size_t p = 2;
    while (has(d, p, 4)) {
        if (d[p] != 0xFF) break;
        const unsigned marker = d[p + 1];
        if (marker == 0xFF) { ++p; continue; }                                       // relleno
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) { p += 2; continue; } // sin longitud
        if (marker == 0xD9 || marker == 0xDA) break;                                 // fin / datos de imagen
        const std::size_t length = u16(d, p + 2, false);
        if (length < 2 || !has(d, p + 2, length)) break;
        const std::size_t data = p + 4;
        const std::size_t size = length - 2;

        if (marker == 0xE0 && startsWith(d, data, "JFIF") && size >= 12) {
            const unsigned units = u8(d, data + 7);
            const double x = u16(d, data + 8, false), y = u16(d, data + 10, false);
            if (units == 1) items.add("format", "format.resolution", "Resolution", dpiText(x, y));
            if (units == 2) items.add("format", "format.resolution", "Resolution", dpiText(x * 2.54, y * 2.54));
        } else if (marker == 0xE1 && startsWith(d, data, "Exif") && size > 6) {
            parseTiff(d, data + 6, data + size, false, items);
        } else if (marker == 0xE1 && startsWith(d, data, "http://ns.adobe.com/xap/")) {
            xmp = true;
        } else if (marker == 0xE2 && startsWith(d, data, "ICC_PROFILE") && size > 14) {
            iccBytes += size - 14;
        } else if (marker == 0xED && startsWith(d, data, "Photoshop 3.0")) {
            iptc = true;
        } else if (marker == 0xFE) {
            items.add("text", "text.comment", "Comment", cleanText(str(d, data, size)));
        } else if (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC && size >= 6) {
            const unsigned components = u8(d, data + 5);
            items.add("format", "format.dimensions", "Dimensions",
                      std::to_string(u16(d, data + 3, false)) + kTimes + std::to_string(u16(d, data + 1, false)) + " px");
            items.add("format", "format.bit_depth", "Bits per channel", std::to_string(u8(d, data)));
            items.add("format", "format.channels", "Channels", std::to_string(components));
            items.add("format", "format.color", "Colour type",
                      components == 1 ? "Grayscale" : components == 3 ? "YCbCr" : components == 4 ? "CMYK" : "");
            const char* mode = marker == 0xC0 ? "Baseline" : marker == 0xC1 ? "Extended sequential"
                             : marker == 0xC2 ? "Progressive" : marker == 0xC3 ? "Lossless" : "Other";
            items.add("format", "format.compression", "Compression", std::string("JPEG, ") + mode);
            if (components == 3 && size >= 6 + 9) {
                const unsigned y = u8(d, data + 7), cb = u8(d, data + 10), cr = u8(d, data + 13);
                if (cb == 0x11 && cr == 0x11) {
                    items.add("format", "format.subsampling", "Chroma subsampling",
                              y == 0x22 ? "4:2:0" : y == 0x21 ? "4:2:2" : y == 0x11 ? "4:4:4" : y == 0x41 ? "4:1:1" : "");
                }
            }
        }
        p += 2 + length;
    }
    if (iccBytes > 0) items.add("format", "format.icc", "Colour profile (ICC)", bytesText(iccBytes));
    if (xmp) items.add("format", "format.xmp", "XMP metadata", "Yes");
    if (iptc) items.add("format", "format.iptc", "IPTC metadata", "Yes");
}

// ---------------------------------------------------------------------------
// PNG
// ---------------------------------------------------------------------------

void parsePng(const Bytes& d, Items& items) {
    std::size_t p = 8;
    int textCount = 0;
    while (has(d, p, 12)) {
        const std::size_t length = u32(d, p, false);
        const std::string type = str(d, p + 4, 4);
        const std::size_t data = p + 8;
        if (!has(d, data, length)) break;

        if (type == "IHDR" && length >= 13) {
            const unsigned colour = u8(d, data + 9);
            items.add("format", "format.dimensions", "Dimensions",
                      std::to_string(u32(d, data, false)) + kTimes + std::to_string(u32(d, data + 4, false)) + " px");
            items.add("format", "format.bit_depth", "Bits per channel", std::to_string(u8(d, data + 8)));
            items.add("format", "format.color", "Colour type",
                      colour == 0 ? "Grayscale" : colour == 2 ? "RGB" : colour == 3 ? "Palette"
                      : colour == 4 ? "Grayscale + alpha" : colour == 6 ? "RGB + alpha" : "");
            items.add("format", "format.compression", "Compression", "Deflate (lossless)");
            if (u8(d, data + 12) == 1) items.add("format", "format.interlace", "Interlacing", "Adam7");
        } else if (type == "pHYs" && length >= 9 && u8(d, data + 8) == 1) {
            items.add("format", "format.resolution", "Resolution",
                      dpiText(u32(d, data, false) * 0.0254, u32(d, data + 4, false) * 0.0254));
        } else if (type == "gAMA" && length >= 4) {
            items.add("format", "format.gamma", "Gamma", fixed(u32(d, data, false) / 100000.0, 5));
        } else if (type == "sRGB") {
            items.add("format", "format.color_space", "Colour space", "sRGB");
        } else if (type == "iCCP") {
            items.add("format", "format.icc", "Colour profile (ICC)", cleanText(str(d, data, std::min<std::size_t>(length, 80)), true));
        } else if (type == "acTL") {
            items.add("format", "format.animation", "Animation", "APNG, " + std::to_string(u32(d, data, false)) + " frames");
        } else if (type == "tIME" && length >= 7) {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u", u16(d, data, false), u8(d, data + 2),
                          u8(d, data + 3), u8(d, data + 4), u8(d, data + 5), u8(d, data + 6));
            items.add("format", "format.time", "Date modified (metadata)", buffer);
        } else if (type == "eXIf") {
            parseTiff(d, data, data + length, false, items);
        } else if ((type == "tEXt" || type == "iTXt" || type == "zTXt") && textCount < 50) {
            const std::string chunk = str(d, data, length);
            const std::size_t nul = chunk.find('\0');
            if (nul != std::string::npos && nul > 0) {
                const std::string keyword = cleanText(chunk.substr(0, nul), true);
                std::string value;
                if (type == "tEXt") {
                    value = cleanText(chunk.substr(nul + 1), true);
                } else if (type == "iTXt" && chunk.size() > nul + 3 && chunk[nul + 1] == 0) {
                    // keyword \0 compresión(2 bytes) idioma \0 clave-traducida \0 texto
                    const std::size_t lang = chunk.find('\0', nul + 3);
                    const std::size_t trans = lang == std::string::npos ? lang : chunk.find('\0', lang + 1);
                    if (trans != std::string::npos) value = cleanText(chunk.substr(trans + 1));
                } else {
                    value = "(compressed)";
                }
                items.add("text", "text." + keyword, keyword, value);
                ++textCount;
            }
        } else if (type == "IEND") {
            break;
        }
        p = data + length + 4; // datos + CRC
    }
}

// ---------------------------------------------------------------------------
// Otros formatos
// ---------------------------------------------------------------------------

void parseBmp(const Bytes& d, Items& items) {
    if (!has(d, 0, 30)) return;
    const std::int32_t w = static_cast<std::int32_t>(u32(d, 18, true));
    const std::int32_t h = static_cast<std::int32_t>(u32(d, 22, true));
    const unsigned compression = u32(d, 30, true);
    items.add("format", "format.dimensions", "Dimensions", std::to_string(w) + kTimes + std::to_string(std::abs(h)) + " px");
    items.add("format", "format.bits_per_pixel", "Bits per pixel", std::to_string(u16(d, 28, true)));
    items.add("format", "format.compression", "Compression",
              compression == 0 ? "None" : compression == 1 ? "RLE 8" : compression == 2 ? "RLE 4"
              : compression == 3 ? "None (bit fields)" : std::to_string(compression));
    items.add("format", "format.resolution", "Resolution", dpiText(u32(d, 38, true) * 0.0254, u32(d, 42, true) * 0.0254));
}

void parseWebp(const Bytes& d, Items& items) {
    std::size_t p = 12;
    bool first = true;
    while (has(d, p, 8)) {
        const std::string type = str(d, p, 4);
        const std::size_t length = u32(d, p + 4, true);
        const std::size_t data = p + 8;
        if (!has(d, data, length)) break;
        if (first && type == "VP8X" && length >= 10) {
            const unsigned flags = u8(d, data);
            const unsigned w = (u8(d, data + 4) | (u8(d, data + 5) << 8) | (u8(d, data + 6) << 16)) + 1;
            const unsigned h = (u8(d, data + 7) | (u8(d, data + 8) << 8) | (u8(d, data + 9) << 16)) + 1;
            items.add("format", "format.dimensions", "Dimensions", std::to_string(w) + kTimes + std::to_string(h) + " px");
            if (flags & 0x10) items.add("format", "format.alpha", "Transparency", "Yes");
            if (flags & 0x02) items.add("format", "format.animation", "Animation", "Yes");
            if (flags & 0x20) items.add("format", "format.icc", "Colour profile (ICC)", "Yes");
            if (flags & 0x04) items.add("format", "format.xmp", "XMP metadata", "Yes");
        } else if (type == "VP8 " && length >= 10) {
            if (first) {
                items.add("format", "format.dimensions", "Dimensions",
                          std::to_string(u16(d, data + 6, true) & 0x3FFF) + kTimes + std::to_string(u16(d, data + 8, true) & 0x3FFF) + " px");
            }
            items.add("format", "format.compression", "Compression", "VP8 (lossy)");
        } else if (type == "VP8L" && length >= 5) {
            if (first) {
                const std::uint32_t bits = u32(d, data + 1, true);
                items.add("format", "format.dimensions", "Dimensions",
                          std::to_string((bits & 0x3FFF) + 1) + kTimes + std::to_string(((bits >> 14) & 0x3FFF) + 1) + " px");
            }
            items.add("format", "format.compression", "Compression", "VP8L (lossless)");
        } else if (type == "EXIF") {
            const std::size_t skip = startsWith(d, data, "Exif") ? 6 : 0;
            parseTiff(d, data + skip, data + length, false, items);
        }
        first = false;
        p = data + length + (length & 1); // los bloques se alinean a 2 bytes
    }
}

/// Identifica el formato por su cabecera y extrae lo que sepa leer.
void parseFormat(const Bytes& d, Items& items) {
    auto type = [&](const char* name) { items.add("format", "format.type", "Format", name); };

    if (has(d, 0, 3) && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF) {
        type("JPEG");
        parseJpeg(d, items);
    } else if (startsWith(d, 0, "\x89PNG\r\n\x1A\n")) {
        type("PNG");
        parsePng(d, items);
    } else if (has(d, 0, 4) && ((d[0] == 'I' && d[1] == 'I' && d[2] == 42 && d[3] == 0) ||
                                (d[0] == 'M' && d[1] == 'M' && d[2] == 0 && d[3] == 42))) {
        type("TIFF");
        parseTiff(d, 0, d.size(), true, items);
    } else if (has(d, 0, 4) && ((d[0] == 'I' && d[1] == 'I' && d[2] == 43) || (d[0] == 'M' && d[1] == 'M' && d[3] == 43))) {
        type("BigTIFF");
    } else if (startsWith(d, 0, "BM")) {
        type("BMP");
        parseBmp(d, items);
    } else if (startsWith(d, 0, "RIFF") && startsWith(d, 8, "WEBP")) {
        type("WebP");
        parseWebp(d, items);
    } else if (startsWith(d, 0, "GIF87a") || startsWith(d, 0, "GIF89a")) {
        type("GIF");
        if (has(d, 0, 10)) {
            items.add("format", "format.dimensions", "Dimensions",
                      std::to_string(u16(d, 6, true)) + kTimes + std::to_string(u16(d, 8, true)) + " px");
        }
    } else if (has(d, 0, 12) && std::memcmp(d.data(), "\0\0\0\x0CjP  ", 8) == 0) {
        type("JPEG 2000");
    } else if (has(d, 0, 4) && d[0] == 0xFF && d[1] == 0x4F && d[2] == 0xFF && d[3] == 0x51) {
        type("JPEG 2000 (codestream)");
    } else if (startsWith(d, 4, "ftyp")) {
        const std::string brand = str(d, 8, 4);
        type(brand == "avif" || brand == "avis" ? "AVIF" : brand.rfind("hei", 0) == 0 || brand == "mif1" ? "HEIF" : "ISO BMFF");
    } else if (has(d, 0, 4) && d[0] == 0x76 && d[1] == 0x2F && d[2] == 0x31 && d[3] == 0x01) {
        type("OpenEXR");
    } else if (startsWith(d, 0, "#?RADIANCE") || startsWith(d, 0, "#?RGBE")) {
        type("Radiance HDR");
    } else if (has(d, 0, 3) && d[0] == 'P' && d[1] >= '1' && d[1] <= '7' && (d[2] == '\n' || d[2] == ' ' || d[2] == '\r')) {
        type("PNM (PBM/PGM/PPM)");
    } else {
        type("Unknown");
    }
}

/// Fecha de modificación del archivo en hora local.
std::string modifiedText(const fs::path& path) {
    std::error_code ec;
    const fs::file_time_type ftime = fs::last_write_time(path, ec);
    if (ec) return "";
    // Conversión de reloj de archivos a reloj del sistema válida en C++17.
    const auto sys = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t tt = std::chrono::system_clock::to_time_t(sys);
    const std::tm* local = std::localtime(&tt);
    if (!local) return "";
    char buffer[32];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) == 0) return "";
    return buffer;
}

} // namespace

std::vector<InfoItem> fileInfo(const std::string& utf8Path) {
    std::vector<InfoItem> result;
    Items items(result);
    try {
        const fs::path path = fromUtf8(utf8Path);
        std::error_code ec;
        const fs::path absolute = fs::absolute(path, ec);
        items.add("file", "file.name", "Name", toUtf8(path.filename()));
        items.add("file", "file.folder", "Folder", toUtf8((ec ? path : absolute).parent_path()));
        const std::uintmax_t size = fs::file_size(path, ec);
        if (!ec) items.add("file", "file.size", "Size on disk", bytesText(size));
        items.add("file", "file.modified", "Modified", modifiedText(path));

        // Los metadatos están al principio del archivo en casi todos los
        // formatos; para archivos enormes se lee solo el primer tramo.
        const std::uintmax_t kLimit = 256u * 1024u * 1024u;
        std::ifstream in(path, std::ios::binary);
        if (in && !ec && size > 0) {
            Bytes bytes(static_cast<std::size_t>(std::min(size, kLimit)));
            in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            bytes.resize(static_cast<std::size_t>(in.gcount()));
            parseFormat(bytes, items);
        }
    } catch (const std::exception&) {
        // Se devuelve lo que se haya podido leer.
    }
    // Los lectores añaden las propiedades en el orden del archivo; se
    // reagrupan conservando ese orden dentro de cada grupo.
    static const char* const kOrder[] = {"file", "format", "exif", "gps", "text"};
    auto rank = [](const InfoItem& item) {
        int i = 0;
        for (const char* group : kOrder) {
            if (item.group == group) break;
            ++i;
        }
        return i;
    };
    std::stable_sort(result.begin(), result.end(),
                     [&](const InfoItem& a, const InfoItem& b) { return rank(a) < rank(b); });
    return result;
}

std::vector<InfoItem> imageInfo(const cv::Mat& image) {
    std::vector<InfoItem> result;
    Items items(result);
    if (image.empty()) return result;

    const int channels = image.channels();
    const int bits = image.depth() == CV_16U ? 16 : 8;
    items.add("image", "image.dimensions", "Dimensions", std::to_string(image.cols) + kTimes + std::to_string(image.rows) + " px");
    items.add("image", "image.megapixels", "Megapixels", fixed(image.cols * static_cast<double>(image.rows) / 1e6, 2) + " MP");
    items.add("image", "image.channels", "Channels",
              std::to_string(channels) + (channels == 1 ? " (gray)" : channels == 3 ? " (B, G, R)" : channels == 4 ? " (B, G, R, A)" : ""));
    items.add("image", "image.bit_depth", "Bits per channel", std::to_string(bits));
    items.add("image", "image.type", "OpenCV type", std::string(bits == 16 ? "CV_16UC" : "CV_8UC") + std::to_string(channels));
    items.add("image", "image.memory", "Memory used", bytesText(image.total() * image.elemSize()));

    try {
        const cv::Mat continuous = image.isContinuous() ? image : image.clone();
        double low = 0.0, high = 0.0;
        cv::minMaxLoc(continuous.reshape(1), &low, &high);
        cv::Scalar mean, deviation;
        cv::meanStdDev(image, mean, deviation);

        static const char* const kNames[4] = {"B", "G", "R", "A"};
        std::string means, deviations;
        for (int c = 0; c < channels && c < 4; ++c) {
            const std::string prefix = channels == 1 ? "" : std::string(kNames[c]) + " ";
            if (c > 0) { means += ", "; deviations += ", "; }
            means += prefix + fixed(mean[c], 2);
            deviations += prefix + fixed(deviation[c], 2);
        }
        items.add("image", "image.range", "Value range (min - max)", fixed(low, 0) + " - " + fixed(high, 0));
        items.add("image", "image.mean", "Mean per channel", means);
        items.add("image", "image.stddev", "Standard deviation per channel", deviations);
    } catch (const cv::Exception&) {
        // Sin estadísticas si OpenCV no puede calcularlas para este tipo.
    }
    return result;
}

std::string infoToText(const std::vector<InfoItem>& items) {
    std::string text, group;
    for (const InfoItem& item : items) {
        if (item.group != group) {
            if (!text.empty()) text += '\n';
            group = item.group;
            text += "[" + group + "]\n";
        }
        text += item.label + ": " + item.value + '\n';
    }
    return text;
}

} // namespace cvcore
