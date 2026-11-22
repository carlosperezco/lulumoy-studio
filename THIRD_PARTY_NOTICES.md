# Third-party software

The code in this repository is distributed under the MIT license (see `LICENSE`). To build
and run, it uses the following libraries, which are **not** included in this repository
and have their own licenses:

| Library | Used for | License | Source code |
|---|---|---|---|
| [OpenCV](https://opencv.org) (4.5 or later) | Image processing (`core/`) | Apache 2.0 | https://github.com/opencv/opencv |
| [GTK 3](https://www.gtk.org) | User interface (`gui/`) | LGPL 2.1 or later | https://gitlab.gnome.org/GNOME/gtk |
| GLib, GdkPixbuf, Pango, Cairo and the other GTK dependencies | Used by GTK; Cairo is also used directly for drawing | LGPL / MPL depending on the library | https://gitlab.gnome.org/GNOME, https://www.cairographics.org |

Both licenses allow these libraries to be used from an MIT-licensed program. The program
links them dynamically (as separate `.dll` or `.so` files) and does not modify their code.

The icon (`assets/lulumoy_icon.svg` and the PNG and ICO files made from it) is an original
design created for this project and is covered by the project's MIT license.

## When distributing the compiled program

The above is enough for the source code. Whoever distributes the compiled program together
with the libraries (the `dist/` folder created by `empaquetar_windows.sh`, or the installer
created with `--instalador`, which contains that same folder) must also:

- include the license text of each bundled library;
- for LGPL libraries, state where their source code can be obtained for the distributed
  version and allow those libraries to be replaced by another version (this is met by
  using separate `.dll` files);
- check the license of **every** bundled `.dll`, not only OpenCV and GTK, because they
  pull in other dependencies.

Some of those dependencies may be under the GPL. For example, the MSYS2 build of libtiff,
used by OpenCV to read TIFF files, depends on jbigkit, which is GPL. The MIT license is
compatible with the GPL, but distributing a bundle that includes a GPL library requires the
corresponding source code to be available. That is why the source code of this program is
published (this repository) before or together with the installer, and why
`dist/licenses/PAQUETES.txt` should be reviewed for each release.

`empaquetar_windows.sh` automatically copies into `dist/licenses/` the licenses of the
MSYS2 packages the `.dll` files come from, and creates `dist/licenses/PAQUETES.txt` with
each package, its version and its license, so it can be reviewed. The installer includes
that folder. The source code of every MSYS2 package is available at
https://github.com/msys2/MINGW-packages and https://packages.msys2.org.

The installer is built with [NSIS](https://nsis.sourceforge.io), under the zlib license,
which places no conditions on the installed program.
