# Lulumoy Studio

**English** · [Español](README.es.md)

A free, modular image editor in C++17, built with **GTK 3** (user interface) and **OpenCV**
(image processing). It runs on Linux and Windows, in a single window with drop-down menus.

- Rotate, flip, crop, and affine and perspective transforms with points picked by mouse
- Filters (Gaussian, median, bilateral, Canny, threshold, morphology...) with live preview
- Pixel rulers around the image and the value of the pixel under the mouse
- Conversion between 8 and 16 bits, grayscale and color
- Black-and-white vectorization to SVG, with preview
- Command chains that can be saved and applied to other images
- File properties and EXIF/GPS metadata
- A math guide for every parameter, with formulas
- Available in English, Spanish and Italian

**Download:** the Windows installer is in the *Releases* section of this repository. On
Linux, build it from source (see [Building](#building)).

Lulumoy Studio is the first application of **DonatellaTech** (https://donatellatech.com), a
project that will bring together several open-source applications.

Dependencies: only GTK 3 and OpenCV (modules `core`, `imgproc`, `imgcodecs`). Everything
else is the C++ standard library: configuration, languages and command chains are plain
text files read by the program's own code.

### The name

The name is inspired by Lulumoy, a deity in the cosmogony of the Pijao people of Tolima
(Colombia), described as the god of wisdom. The project does not claim to represent the
deity or to speak for the Pijao people: the name is a tribute to that cultural heritage.

## Architecture

```
app/main.cpp            Entry point: resources, configuration, languages, startup
app/version.h.in        Version header template (filled in by CMake)
app/lulumoy.rc.in       Icon and version information for the Windows .exe
core/  -> libcvcore     Operations (OpenCV). Knows nothing about the interface
  include/cvcore/       operation.h, registry.h, params.h, pipeline.h, image_io.h,
                        image_info.h, vectorize.h, error.h
  src/                  registry, chains, reading/writing
  modules/              ONE FILE PER MODULE: each registers itself
gui/   -> libcvgui      Interface (GTK 3). Contains no operations
  include/cvgui/        gui.h, i18n.h, app_config.h
  src/                  main window, viewer with rulers, dialogs, help
config/app.conf         Title, icon, logo, welcome message, language
lang/*.lang             Translations (es, en, it)
help/<language>/        Math guide (HTML with MathML formulas)
tools/ayuda/            Guide generator (Python)
assets/                 Logo and icon (SVG, PNG and ICO)
```

Flow: `main` loads the configuration and languages and calls `cvgui::runGui`. The window
walks through `cvcore::Registry` and creates a menu item for each operation. When one is
chosen, it builds the parameter dialog from `Operation::params()`, calls
`Operation::apply()` and adds the step to the command chain.

## Building

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake pkg-config libgtk-3-dev libopencv-dev
cmake -S . -B build
cmake --build build -j
./build/bin/lulumoy
```

### Windows (MSYS2, UCRT64 terminal)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-gtk3 mingw-w64-ucrt-x86_64-opencv
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/lulumoy.exe
```

Built this way, it only starts from the MSYS2 terminal, which is where Windows finds the
GTK and OpenCV DLLs. To run it with a double click or take it to another PC, create a
self-contained folder with the executable, its DLLs and the GTK resources:

```bash
./empaquetar_windows.sh        # creates dist/; the program is dist/bin/lulumoy.exe
```

### Windows installer

```bash
./empaquetar_windows.sh --instalador   # creates dist/ and instalador/LulumoyStudio-<version>-win64.exe
```

The installer is built with [NSIS](https://nsis.sourceforge.io) (zlib license) through
CPack. It installs the program in `Program Files\DonatellaTech\Lulumoy Studio`, shows the
license, creates a Start menu shortcut (and, if selected, a desktop shortcut) and can be
uninstalled from "Installed apps". NSIS is required: the official installer is
recommended, because the MSYS2 package (`mingw-w64-ucrt-x86_64-nsis`, version 3.13) has a
known bug that prevents building these installers ("Plugin not found"). The script uses
the official one if it finds it.

With `-DLULUMOY_HIDE_CONSOLE=OFF` the console window is kept, which is needed to see the
output of `--list` and `--apply`.

The executable, `libcvcore` and the folders `config/`, `lang/`, `assets/` and `help/` end
up in `build/bin`. Every build copies those folders again: edit the originals, not the copy.

## Usage

| Menu | Functions |
|---|---|
| File | Open, Save as (always a new file, never overwrites), Export SVG (vectorize), Properties, Close image, Quit |
| Edit | Undo last step, Restore original, Rotate, Flip, Crop, Affine transform (3 points) and perspective transform (4 points) with the mouse |
| Edit > Change format | 8 bits, 16 bits, Grayscale, Color |
| Edit > Filters | Gaussian, box, median, bilateral, sharpen, Canny, threshold, brightness/contrast, morphology |
| Chain | View current chain, Save chain, Apply chain from file |
| View | Zoom in, zoom out, 100 %, fit to window |
| Language | One item for each file in `lang/` |
| Help | Mathematical guide to parameters (F1), DonatellaTech: mission and vision, About |

16-bit images can only be saved as PNG or TIFF.

### Rulers and coordinate system

Two rulers graduated in pixels surround the image: the x axis at the top and the y axis on
the left. The origin (0, 0) is the top-left corner; x grows to the right and y grows
downwards, as in OpenCV. Tick x is the left edge of pixel x, so pixel (x, y) covers the
square from x to x+1 and from y to y+1. The rulers follow zoom and scrolling, and a red
line marks the mouse position. The status bar shows the pixel under the mouse and its
value (RGB, RGBA or gray level; 0-255 for 8 bits, 0-65535 for 16 bits). At 800 % zoom or
more, the pixel grid is drawn as well.

### Preview

The parameter dialogs (rotate, flip, crop, affine and perspective transforms, filters) show
a preview, with the same rulers, that is recomputed when a parameter changes. It can show
the result or the original, fitted to the window or at 100 %; with *Original*, the crop
area is marked as a rectangle and, for the transforms, the chosen source points are
marked. Below it are the size of the result, the computation time and the pixel under the
mouse.

The preview applies the operation to the full image, so it is exactly what you will get
(nothing is recomputed when you accept). With very large images and slow filters such as
the bilateral filter it may take a while: the *Preview* check box turns it off, and the
program remembers the choice. With *Fit*, the image is shown reduced: to judge a filter,
look at it at 100 %.

### Transforms, SVG and properties

*Edit > Affine transform* asks for six clicks on the image: for each of the three pairs,
first the source point (yellow) and then where it should end up (cyan). Right click or
Backspace removes the last point and Esc cancels. Set the zoom beforehand, because the
menus are disabled while the points are being picked.

*Edit > Perspective transform* works the same way with four pairs (eight clicks) and is
useful, for example, to straighten a document photographed at an angle: mark each corner
and then the place where it should go.

*File > Export SVG (vectorize)* converts the current image into black-and-white vector
paths. The dialog shows the parameters next to a preview that updates when they change,
and can display the vector result, an overlay on the image or the binarized image; below
it shows the number of paths, the number of nodes and the approximate SVG size.

| Parameter | Effect |
|---|---|
| Threshold method | Fixed, automatic (Otsu) or adaptive (for uneven lighting or shadowed scans) |
| Working resolution | ×2 or ×4 give more precise edges on small images; reduced automatically on very large ones |
| Pre-blur and edge cleanup | Remove noise and jagged edges before tracing |
| Ignore specks | Discards shapes and holes smaller than the given area |
| Simplification | Tolerance in pixels; higher means fewer nodes and less detail |
| Outline and corners | Polygons or smooth curves; turns sharper than the given angle are kept as corners |

The outline follows the outer edge of the pixels: without simplification, the SVG
reproduces the binarized image pixel by pixel, including 1-px lines. The SVG uses a single
black path with the even-odd fill rule, so holes (the inside of an "o") stay transparent.
As with images, an existing file is never overwritten.

*File > Properties* shows the file data (size, date, actual format, dimensions,
compression, resolution), the EXIF and GPS metadata of JPEG, PNG, TIFF and WebP files,
embedded text and statistics of the image in memory. The metadata is read by the program's
own code (`core/src/image_info.cpp`), without additional libraries. When saving, the
application does not keep the metadata of the original file.

### Command chains

Every operation applied is added to the chain of the open image. *Chain > Save chain*
writes it to a `.cvchain` file and *Apply chain from file* runs it on another image. It is
plain text and can be edited by hand:

```
# lulumoy-chain 1
edit.rotate angle=90 expand=1
filter.gaussian ksize=5 sigma=1.5
format.togray
```

Also without the interface:

```bash
lulumoy --list                                  # operations and parameters
lulumoy --info image.jpg                        # properties and metadata
lulumoy --svg image.png drawing.svg epsilon=0.5 # vectorize (no parameters: list of keys)
lulumoy --apply chain.cvchain input.png output.png
lulumoy --version
```

### Math guide

*Help > Mathematical guide to parameters* (F1) and the **Help** button of each parameter
dialog open a page in the browser that explains what the operation computes and how each
parameter affects it, with the formulas of the OpenCV functions the program uses: filters,
rotation, affine and perspective transforms, interpolation, format changes and each step
of the SVG vectorization. It is available in English, Spanish and Italian, following the
active language, and works offline: the formulas are written in MathML, which current
browsers display without additional libraries.

The pages are generated with `python tools/ayuda/gen.py`. The texts are in
`tools/ayuda/pages_*.py` and the parameter names are read from `lang/*.lang`, so they always
match the dialogs. When adding a module with parameters, add its page there and generate
again; if an operation has no page, its dialog does not show the Help button.

## Configuration (`config/app.conf`)

```
window_title = Lulumoy Studio     # window title
icon = assets/lulumoy_icon.png   # window icon
welcome_logo = assets/logo.png   # logo on the welcome screen
welcome_message.en = Welcome\nFile > Open to start
language = es                    # es, en, it
```

Relative paths start from the folder that contains `config/`. The `LULUMOY_HOME`
environment variable can point to a different resource folder.

Settings changed from the program (the language) are saved in the user's folder and take
precedence over `app.conf`, because the program folder may not be writable:
`%APPDATA%\DonatellaTech\Lulumoy Studio\settings.conf` on Windows and
`~/.config/donatellatech/lulumoy/settings.conf` on Linux.

## Adding a module

Create a file in `core/modules/`, for example `invert.cpp`, and build again. Neither the
interface nor CMake needs to be touched:

```cpp
#include <opencv2/core.hpp>
#include "cvcore/registry.h"

namespace {
using namespace cvcore;

/// Image negative.
class Invert final : public Operation {
public:
    std::string id() const override { return "filter.invert"; }
    std::string menu() const override { return "edit/filters"; }   // or a new menu: "tools"
    std::string label() const override { return "Invert"; }
    std::vector<ParamSpec> params() const override {
        return {ParamSpec::boolean("enabled", "Enabled", true)};    // optional
    }
    cv::Mat apply(const cv::Mat& in, const ParamValues& p) const override {
        if (!getBool(p, "enabled")) return in.clone();
        cv::Mat out;
        cv::bitwise_not(in, out);
        return out;
    }
};
CVCORE_REGISTER_OPERATION(Invert)
} // namespace
```

Optional translation in each `lang/*.lang` (if missing, the module's text is used):

```
op.filter.invert = Invert
op.filter.invert.enabled = Enabled
```

A module can ask for points on the image with `ParamSpec::point("p1", "First point")`. The
interface requests them with mouse clicks, in the declared order, and the module reads
them as `getReal(p, "p1.x")` and `getReal(p, "p1.y")`, in pixels. `core/modules/affine.cpp`
and `perspective.cpp` are the examples.

To have the preview mark an area on the original image (like the crop rectangle), a
module can implement `region()`, which returns the vertices of a polygon in input-image
coordinates.

Conventions: images are 8- or 16-bit unsigned with 1, 3 (BGR) or 4 (BGRA) channels;
`apply()` returns a new image and throws `cvcore::Error` if it cannot operate.

## Adding a language

Copy `lang/en.lang` to `lang/<code>.lang` (for example `fr.lang`), translate the values and
change `lang.name`. It appears in the Language menu at startup.

## Versions

The version follows the MAJOR.MINOR.PATCH scheme ([semantic versioning](https://semver.org)).
Current version: **1.0.0**. The changes in each version are listed in
[CHANGELOG.md](CHANGELOG.md).

| Change | Example | When |
|---|---|---|
| PATCH | 1.0.0 → 1.0.1 | Bug fixes |
| MINOR | 1.0.1 → 1.1.0 | New features that do not change existing behavior |
| MAJOR | 1.4.0 → 2.0.0 | Incompatible changes (for example, in the chain format or the module interface) |

The version is set in a single place, the `project(lulumoy VERSION 1.0.0 ...)` line in
`CMakeLists.txt`. The About dialog, `lulumoy --version`, the installer name, the Windows
`.exe` properties and the Doxygen documentation all take it from there. When releasing a
version, tag it in git:

```bash
git tag -a v1.0.0 -m "Lulumoy Studio 1.0.0"
```

## Documentation (Doxygen)

All the code is documented in Doxygen format (the comments are in Spanish).

```bash
doxygen Doxyfile                       # output in docs/html/index.html
cmake --build build --target docs      # same, if CMake found Doxygen (includes the version)
```

## License

This project is distributed under the [MIT](LICENSE) license: it can be used, copied,
modified and redistributed freely, including for commercial purposes, as long as the
copyright notice is kept. It comes with no warranty.

It uses [OpenCV](https://opencv.org) (Apache 2.0) and [GTK 3](https://www.gtk.org) (LGPL 2.1
or later), which are not included in the repository. The details, and what to take into
account when distributing the compiled program, are in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Credits

The icon (`assets/lulumoy_icon.svg`, a turtle with a camera) is an original design created
for this project and is distributed under the same MIT license.

Developed with the help of Claude (Anthropic) as a programming assistant.

## Supporting the project

If you find this program useful, you can support it with a donation through GitHub's
"Sponsor" button. It is entirely voluntary.
