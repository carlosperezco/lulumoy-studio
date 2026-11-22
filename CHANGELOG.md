# Changelog

All notable changes to Lulumoy Studio are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[semantic versioning](https://semver.org).

## 1.0.0 - 2026-11-22

First public release.

### Image editing
- Open images and save them always as a new file (an existing file is never overwritten).
- Rotate, flip and crop.
- Affine transform (3 points) and perspective transform (4 points), with the points picked
  by mouse on the image.
- Format conversion: 8 bits, 16 bits, grayscale, color.
- Filters: Gaussian, box, median, bilateral, sharpen, Canny, threshold,
  brightness/contrast, morphology.
- Undo last step and restore the original image.

### Viewing
- Pixel rulers around the image (x axis at the top, y axis on the left, origin at the
  top-left corner), following zoom and scrolling.
- Position and value of the pixel under the mouse in the status bar.
- Pixel grid at 800 % zoom or more.
- Live preview in the parameter dialogs, showing the result or the original, with the
  crop area or the transform's source points marked on the original.

### Vectorization
- Black-and-white vectorization to SVG with live preview: fixed, Otsu or adaptive
  threshold, working resolution, noise cleanup, simplification, polygons or smooth curves.

### Command chains
- Every operation is recorded in a chain that can be saved as a `.cvchain` text file and
  applied to other images.
- Command-line modes: `--list`, `--info`, `--svg`, `--apply`, `--version`.

### Information and help
- File properties with EXIF and GPS metadata (JPEG, PNG, TIFF, WebP).
- Mathematical guide to every parameter, with MathML formulas, opened in the browser
  (F1 or the Help button of each dialog).
- About dialog and DonatellaTech mission and vision.

### Platform
- Windows (MSYS2 UCRT64) and Linux.
- English, Spanish and Italian interface; new languages are added with a text file.
- Modules in `core/modules/` register themselves and appear in the menus automatically.
- Windows installer (NSIS) and self-contained folder with the licenses of the bundled
  libraries.
- Configurable title, icon, welcome logo and message; user settings stored in the user's
  folder.

