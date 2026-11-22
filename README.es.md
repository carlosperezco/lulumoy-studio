# Lulumoy Studio

[English](README.md) · **Español**

Editor de imágenes libre y modular en C++17 con **GTK 3** (interfaz) y **OpenCV**
(procesamiento). Funciona en Linux y Windows, en una sola ventana con menús desplegables.

- Rotar, voltear, recortar y transformaciones afín y de perspectiva eligiendo los puntos con el ratón
- Filtros (gaussiano, mediana, bilateral, Canny, umbral, morfología...) con vista previa en vivo
- Reglas en píxeles alrededor de la imagen y valor del píxel bajo el ratón
- Conversión entre 8 y 16 bits, escala de grises y color
- Vectorización en blanco y negro a SVG, con vista previa
- Cadenas de comandos que se guardan y se aplican a otras imágenes
- Propiedades y metadatos EXIF/GPS
- Guía matemática de cada parámetro, con fórmulas
- En español, inglés e italiano

**Descarga:** el instalador para Windows está en la sección *Releases* de este repositorio.
En Linux se compila desde el código (ver [Compilar](#compilar)).

Lulumoy Studio es la primera aplicación de **DonatellaTech** (https://donatellatech.com), un
proyecto que reunirá varias aplicaciones de código abierto.

Dependencias: solo GTK 3 y OpenCV (módulos `core`, `imgproc`, `imgcodecs`). El resto es
la librería estándar de C++: configuración, idiomas y cadenas de comandos son archivos de
texto leídos con código propio.

### El nombre

El nombre se inspira en Lulumoy, deidad de la cosmogonía del pueblo pijao del Tolima
(Colombia), descrita como dios de la sabiduría. El proyecto no pretende representar a la
deidad ni hablar en nombre del pueblo pijao: el nombre es un homenaje a esa herencia
cultural.

## Arquitectura

```
app/main.cpp            Gestión central: recursos, configuración, idiomas, arranque
app/version.h.in        Plantilla de la cabecera de versión (la rellena CMake)
app/lulumoy.rc.in       Icono y datos de versión del .exe de Windows
core/  -> libcvcore     Operaciones (OpenCV). No sabe nada de la interfaz
  include/cvcore/       operation.h, registry.h, params.h, pipeline.h, image_io.h,
                        image_info.h, vectorize.h, error.h
  src/                  registro, cadenas, lectura/escritura
  modules/              UN ARCHIVO POR MÓDULO: se registran solos
gui/   -> libcvgui      Interfaz (GTK 3). No contiene ninguna operación
  include/cvgui/        gui.h, i18n.h, app_config.h
  src/                  ventana principal, visor con reglas, diálogos, ayuda
config/app.conf         Título, icono, logo, mensaje inicial, idioma
lang/*.lang             Traducciones (es, en, it)
help/<idioma>/          Guía matemática (HTML con fórmulas MathML)
tools/ayuda/            Generador de la guía (Python)
assets/                 Logo e icono (SVG, PNG e ICO)
```

Flujo: `main` carga configuración e idiomas y llama a `cvgui::runGui`. La ventana recorre
`cvcore::Registry` y crea un elemento de menú por cada operación. Al elegir una, genera el
diálogo de parámetros a partir de `Operation::params()`, llama a `Operation::apply()` y
añade el paso a la cadena de comandos.

## Compilar

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake pkg-config libgtk-3-dev libopencv-dev
cmake -S . -B build
cmake --build build -j
./build/bin/lulumoy
```

### Windows (MSYS2, terminal UCRT64)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-gtk3 mingw-w64-ucrt-x86_64-opencv
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/lulumoy.exe
```

Así compilado solo arranca desde la terminal de MSYS2, que es donde Windows encuentra las
DLL de GTK y OpenCV. Para abrirlo con doble clic o llevarlo a otro PC, cree una carpeta
autónoma con el ejecutable, sus DLL y los recursos de GTK:

```bash
./empaquetar_windows.sh        # crea dist/; el programa queda en dist/bin/lulumoy.exe
```

### Instalador de Windows

```bash
./empaquetar_windows.sh --instalador   # crea dist/ y instalador/LulumoyStudio-<versión>-win64.exe
```

El instalador se genera con [NSIS](https://nsis.sourceforge.io) (licencia zlib) a través de
CPack. Instala el programa en `Archivos de programa\DonatellaTech\Lulumoy Studio`, muestra la
licencia, crea el acceso del menú Inicio (y, si se marca, del escritorio) y se desinstala
desde "Aplicaciones instaladas". Hace falta NSIS: se recomienda el instalador oficial, porque
el paquete de MSYS2 (`mingw-w64-ucrt-x86_64-nsis`, versión 3.13) tiene un fallo conocido que
impide compilar estos instaladores ("Plugin not found"). El script usa el oficial si lo
encuentra.

Con `-DLULUMOY_HIDE_CONSOLE=OFF` se conserva la consola, necesaria para ver la salida de
`--list` y `--apply`.

El ejecutable, `libcvcore` y las carpetas `config/`, `lang/`, `assets/` y `help/` quedan en
`build/bin`. La compilación vuelve a copiar esas carpetas: edite los originales, no la copia.

## Uso

| Menú | Funciones |
|---|---|
| Archivo | Abrir, Guardar como (siempre archivo nuevo, nunca sobrescribe), Exportar SVG (vectorizar), Propiedades, Cerrar imagen, Salir |
| Edición | Deshacer último paso, Restaurar original, Rotar, Voltear, Recortar, Transformación afín (3 puntos) y de perspectiva (4 puntos) con el ratón |
| Edición > Cambiar formato | 8 bits, 16 bits, Escala de grises, Color |
| Edición > Filtros | Gaussiano, media, mediana, bilateral, enfocar, Canny, umbral, brillo/contraste, morfología |
| Cadena | Ver cadena actual, Guardar cadena, Aplicar cadena desde archivo |
| Ver | Acercar, alejar, 100 %, ajustar a la ventana |
| Idioma | Un elemento por cada archivo de `lang/` |
| Ayuda | Guía matemática de parámetros (F1), misión y visión de DonatellaTech, Acerca de |

Las imágenes de 16 bits solo se pueden guardar en PNG o TIFF.

### Reglas y sistema de coordenadas

Alrededor de la imagen hay dos reglas graduadas en píxeles: arriba el eje x y a la izquierda
el eje y. El origen (0, 0) es la esquina superior izquierda; x crece hacia la derecha e y
hacia abajo, como en OpenCV. La marca x es el borde izquierdo del píxel x, así que el píxel
(x, y) ocupa el cuadrado de x a x+1 y de y a y+1. Las reglas siguen al zoom y al
desplazamiento, y una línea roja indica la posición del ratón. La barra de estado muestra
el píxel que hay bajo el ratón y su valor (RGB, RGBA o nivel de gris; 0-255 en 8 bits,
0-65535 en 16 bits). Con zoom de 800 % o más se dibuja también la rejilla de píxeles.

### Vista previa

Los diálogos de parámetros (rotar, voltear, recortar, transformaciones afín y de
perspectiva, filtros) muestran una vista previa, con las mismas reglas, que se recalcula
al cambiar un parámetro. Se puede ver el resultado o el original, ajustado a la ventana o
al 100 %; con *Original*, el recorte se marca como un rectángulo y en las transformaciones
se marcan los puntos de origen elegidos. Debajo se indican el tamaño del resultado, el
tiempo de cálculo y el píxel bajo el ratón.

La vista previa aplica la operación a la imagen completa, para que sea exactamente lo que
se obtendrá (al aceptar no se vuelve a calcular). En imágenes muy grandes y con filtros
lentos, como el bilateral, puede tardar: la casilla *Vista previa* la desactiva, y el
programa recuerda la elección. Con *Ajustar*, la imagen se ve reducida: para juzgar un
filtro conviene mirar al 100 %.

### Transformaciones, SVG y propiedades

*Edición > Transformación afín* pide seis clics sobre la imagen: para cada uno de los tres
pares, primero el punto de origen (amarillo) y después dónde debe quedar (cian). Clic
derecho o Retroceso borran el último punto y Esc cancela. Conviene ajustar el zoom antes,
porque los menús se desactivan mientras se eligen los puntos.

*Edición > Transformación de perspectiva* funciona igual con cuatro pares (ocho clics) y
sirve, por ejemplo, para enderezar un documento fotografiado en ángulo: se marca cada
esquina y después el lugar donde debe quedar.

*Archivo > Exportar SVG (vectorizar)* convierte la imagen actual en trazados vectoriales en
blanco y negro. El diálogo muestra los parámetros junto a una vista previa que se actualiza
al cambiarlos, y puede verse como resultado vectorial, superpuesto a la imagen o como imagen
binarizada; debajo indica trazados, nodos y tamaño aproximado del SVG.

| Parámetro | Efecto |
|---|---|
| Método de umbral | Fijo, automático (Otsu) o adaptativo (para iluminación desigual o escaneos con sombra) |
| Resolución de trabajo | ×2 o ×4 dan bordes más precisos en imágenes pequeñas; se reduce sola en imágenes muy grandes |
| Desenfoque previo y limpieza de bordes | Eliminan ruido y dientes antes de trazar |
| Ignorar manchas | Descarta formas y agujeros de menos área que la indicada |
| Simplificación | Tolerancia en píxeles; más alta, menos nodos y menos detalle |
| Contorno y esquinas | Polígonos o curvas suaves; los giros mayores que el ángulo indicado quedan como esquina |

El contorno sigue el borde exterior de los píxeles: sin simplificación, el SVG reproduce la
imagen binarizada píxel a píxel, incluidas líneas de 1 px. El SVG usa un único trazado negro
con regla de relleno par-impar, así que los agujeros (el interior de una "o") quedan
transparentes. Como las imágenes, nunca sobrescribe un archivo existente.

*Archivo > Propiedades* muestra los datos del archivo (tamaño, fecha, formato real,
dimensiones, compresión, resolución), los metadatos EXIF y GPS de JPEG, PNG, TIFF y WebP,
los textos incrustados y las estadísticas de la imagen en memoria. Los metadatos se leen
con código propio (`core/src/image_info.cpp`), sin librerías adicionales. Al guardar, la
aplicación no conserva los metadatos del archivo original.

### Cadenas de comandos

Cada operación aplicada se añade a la cadena de la imagen abierta. *Cadena > Guardar
cadena* la escribe en un archivo `.cvchain` y *Aplicar cadena desde archivo* la ejecuta
sobre otra imagen. Es texto plano y se puede editar a mano:

```
# lulumoy-chain 1
edit.rotate angle=90 expand=1
filter.gaussian ksize=5 sigma=1.5
format.togray
```

También sin interfaz:

```bash
lulumoy --list                                  # operaciones y parámetros
lulumoy --info imagen.jpg                       # propiedades y metadatos
lulumoy --svg imagen.png dibujo.svg epsilon=0.5 # vectorizar (sin parámetros: lista de claves)
lulumoy --apply cadena.cvchain entrada.png salida.png
```

### Guía matemática

*Ayuda > Guía matemática de parámetros* (F1) y el botón **Ayuda** de cada diálogo de
parámetros abren en el navegador una página que explica qué calcula la operación y cómo
influye cada parámetro, con las fórmulas de las funciones de OpenCV que usa el programa:
filtros, rotación, transformaciones afín y de perspectiva, interpolación, cambios de formato
y cada paso de la vectorización a SVG. Está en español, inglés e italiano, según el idioma
activo, y funciona sin conexión: las fórmulas están en MathML, que los navegadores actuales
muestran sin librerías adicionales.

Las páginas se generan con `python tools/ayuda/gen.py`. Los textos están en
`tools/ayuda/pages_*.py` y los nombres de los parámetros se leen de `lang/*.lang`, así que
siempre coinciden con los diálogos. Al añadir un módulo con parámetros, añada su página allí
y vuelva a generar; si una operación no tiene página, su diálogo no muestra el botón Ayuda.

## Configuración (`config/app.conf`)

```
window_title = Lulumoy Studio     # nombre de la ventana
icon = assets/lulumoy_icon.png   # icono de la ventana
welcome_logo = assets/logo.png   # logo de la pantalla inicial
welcome_message.es = Bienvenido\nArchivo > Abrir para empezar
language = es                    # es, en, it
```

Las rutas relativas parten de la carpeta que contiene `config/`. La variable de entorno
`LULUMOY_HOME` permite indicar otra carpeta de recursos.

Lo que se cambia desde el programa (el idioma) se guarda en la carpeta del usuario y tiene
prioridad sobre `app.conf`, porque la carpeta del programa puede no admitir escritura:
`%APPDATA%\DonatellaTech\Lulumoy Studio\settings.conf` en Windows y
`~/.config/donatellatech/lulumoy/settings.conf` en Linux.

## Añadir un módulo

Cree un archivo en `core/modules/`, por ejemplo `invert.cpp`, y vuelva a compilar. No hay
que tocar la interfaz ni CMake:

```cpp
#include <opencv2/core.hpp>
#include "cvcore/registry.h"

namespace {
using namespace cvcore;

/// Negativo de la imagen.
class Invert final : public Operation {
public:
    std::string id() const override { return "filter.invert"; }
    std::string menu() const override { return "edit/filters"; }   // o un menú nuevo: "tools"
    std::string label() const override { return "Invert"; }
    std::vector<ParamSpec> params() const override {
        return {ParamSpec::boolean("enabled", "Enabled", true)};    // opcional
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

Traducción opcional en cada `lang/*.lang` (si falta, se usa el texto del módulo):

```
op.filter.invert = Invertir
op.filter.invert.enabled = Activado
```

Un módulo puede pedir puntos de la imagen con `ParamSpec::point("p1", "First point")`. La
interfaz los solicita con clics del ratón, en el orden declarado, y el módulo los lee como
`getReal(p, "p1.x")` y `getReal(p, "p1.y")`, en píxeles. `core/modules/affine.cpp` y
`perspective.cpp` son los ejemplos.

Para que la vista previa marque una zona sobre la imagen original (como el rectángulo del
recorte), el módulo puede implementar `region()`, que devuelve los vértices de un polígono
en coordenadas de la imagen de entrada.

Convenciones: las imágenes son de 8 o 16 bits sin signo con 1, 3 (BGR) o 4 (BGRA)
canales; `apply()` devuelve una imagen nueva y lanza `cvcore::Error` si no puede operar.

## Añadir un idioma

Copie `lang/en.lang` a `lang/<código>.lang` (por ejemplo `fr.lang`), traduzca los valores y
cambie `lang.name`. Aparece en el menú Idioma al arrancar.

## Versiones

La versión sigue el esquema MAYOR.MENOR.PARCHE ([versionado semántico](https://semver.org/lang/es/)).
Versión actual: **1.0.0**. Los cambios de cada versión están en [CHANGELOG.md](CHANGELOG.md).

| Cambio | Ejemplo | Cuándo |
|---|---|---|
| PARCHE | 1.0.0 → 1.0.1 | Correcciones de errores |
| MENOR | 1.0.1 → 1.1.0 | Funciones nuevas que no cambian lo que ya existía |
| MAYOR | 1.4.0 → 2.0.0 | Cambios incompatibles (por ejemplo, en el formato de las cadenas o en la interfaz de los módulos) |

La versión se cambia en un único sitio, la línea `project(lulumoy VERSION 1.0.0 ...)` de
`CMakeLists.txt`. De ahí salen el "Acerca de", `lulumoy --version`, el nombre del instalador,
las propiedades del `.exe` en Windows y la documentación Doxygen. Al publicar una versión,
conviene marcarla en git:

```bash
git tag -a v1.0.0 -m "Lulumoy Studio 1.0.0"
```

## Documentación (Doxygen)

Todo el código está comentado en formato Doxygen.

```bash
doxygen Doxyfile                       # resultado en docs/html/index.html
cmake --build build --target docs      # equivalente, si CMake encontró Doxygen (incluye la versión)
```

## Licencia

Este proyecto se distribuye con licencia [MIT](LICENSE): se puede usar, copiar, modificar y
redistribuir libremente, también con fines comerciales, conservando el aviso de copyright.
Se ofrece sin ninguna garantía.

Usa [OpenCV](https://opencv.org) (Apache 2.0) y [GTK 3](https://www.gtk.org) (LGPL 2.1 o
posterior), que no se incluyen en el repositorio. Los detalles, y lo que hay que tener en
cuenta al distribuir el programa compilado, están en
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) (en inglés).

## Créditos

El icono (`assets/lulumoy_icon.svg`, una tortuga con una cámara de fotos) es un diseño
original creado para este proyecto y se distribuye con la misma licencia MIT.

Desarrollado con la ayuda de Claude (Anthropic) como asistente de programación.

## Apoyar el proyecto

Si este programa le resulta útil, puede apoyarlo con una donación desde el botón
"Sponsor" de GitHub. Es completamente voluntario.
