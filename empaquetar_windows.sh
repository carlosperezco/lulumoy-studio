#!/usr/bin/env bash
#
# Crea una carpeta "dist/" autónoma para Windows: el ejecutable, todas las DLL
# que necesita y los recursos de GTK. La carpeta resultante se puede abrir con
# doble clic y copiar a otro PC sin instalar MSYS2.
#
# Con --instalador crea además un instalador .exe con NSIS en instalador/.
#
# Uso (desde la terminal MSYS2 UCRT64, después de compilar):
#     ./empaquetar_windows.sh                # usa build/ y crea dist/
#     ./empaquetar_windows.sh --instalador   # dist/ y el instalador
#     ./empaquetar_windows.sh build dist     # carpetas indicadas a mano
#
set -euo pipefail

INSTALADOR=0
argumentos=()
for argumento in "$@"; do
    case "$argumento" in
        --instalador) INSTALADOR=1 ;;
        *) argumentos+=("$argumento") ;;
    esac
done
set -- ${argumentos[@]+"${argumentos[@]}"}

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="${1:-$ROOT/build}"
DIST="${2:-$ROOT/dist}"
PREFIX="${MINGW_PREFIX:-}"

if [ -z "$PREFIX" ]; then
    echo "Error: ejecute este script desde la terminal MSYS2 UCRT64." >&2
    exit 1
fi
if [ ! -f "$BUILD/bin/lulumoy.exe" ]; then
    echo "Error: no existe $BUILD/bin/lulumoy.exe. Compile primero con: cmake --build build" >&2
    exit 1
fi

echo "Entorno:  $PREFIX"
echo "Origen:   $BUILD/bin"
echo "Destino:  $DIST"

rm -rf "$DIST"
mkdir -p "$DIST/bin" "$DIST/lib" "$DIST/share/glib-2.0/schemas" "$DIST/share/icons"

# --- 1. Programa y recursos propios -----------------------------------------
cp "$BUILD/bin/lulumoy.exe" "$DIST/bin/"
mkdir -p "$DIST/licenses"
for doc in LICENSE THIRD_PARTY_NOTICES.md README.md README.es.md CHANGELOG.md; do
    [ -f "$ROOT/$doc" ] && cp "$ROOT/$doc" "$DIST/"
done
cp "$BUILD/bin/"*cvcore*.dll "$DIST/bin/"
for dir in config lang assets help; do
    cp -r "$ROOT/$dir" "$DIST/bin/"
done

# --- 2. Recursos de GTK ------------------------------------------------------
# Cargadores de imágenes (PNG, JPEG, SVG...) que usa GTK para logo e iconos.
if [ -d "$PREFIX/lib/gdk-pixbuf-2.0" ]; then
    cp -r "$PREFIX/lib/gdk-pixbuf-2.0" "$DIST/lib/"
fi
# Ajustes compilados de GLib/GTK (los necesita el diálogo de archivos).
if [ -f "$PREFIX/share/glib-2.0/schemas/gschemas.compiled" ]; then
    cp "$PREFIX/share/glib-2.0/schemas/gschemas.compiled" "$DIST/share/glib-2.0/schemas/"
fi
# Temas de iconos.
for theme in Adwaita hicolor; do
    if [ -d "$PREFIX/share/icons/$theme" ]; then
        cp -r "$PREFIX/share/icons/$theme" "$DIST/share/icons/"
    fi
done

# --- 3. DLL de las que depende todo lo anterior ------------------------------
# Se lee la tabla de importación de cada archivo con objdump (no ejecuta el
# programa) y se sigue la cadena: las DLL de MSYS2 se copian y se analizan a su
# vez. Las de C:\Windows no están en $PREFIX/bin y se ignoran.
if ! command -v objdump >/dev/null 2>&1; then
    echo "Error: no se encuentra objdump. Instálelo con: pacman -S mingw-w64-ucrt-x86_64-binutils" >&2
    exit 1
fi

declare -A vistas=()
pendientes=()
copiadas=0

analizar() {
    local nombre clave
    while read -r nombre; do
        [ -n "$nombre" ] || continue
        clave="${nombre,,}"
        [ -z "${vistas[$clave]:-}" ] || continue
        vistas[$clave]=1
        if [ -f "$PREFIX/bin/$nombre" ]; then
            cp "$PREFIX/bin/$nombre" "$DIST/bin/"
            pendientes+=("$PREFIX/bin/$nombre")
            copiadas=$((copiadas + 1))
        fi
    done < <(objdump -p "$1" 2>/dev/null | awk '/DLL Name:/ { print $3 } /^ *NEEDED/ { print $2 }' | tr -d '\r')
}

analizar "$DIST/bin/lulumoy.exe"
for dll in "$DIST/bin/"*cvcore*.dll; do
    analizar "$dll"
done
if [ -d "$DIST/lib/gdk-pixbuf-2.0" ]; then
    while read -r cargador; do
        analizar "$cargador"
    done < <(find "$DIST/lib/gdk-pixbuf-2.0" -name '*.dll')
fi
while [ "${#pendientes[@]}" -gt 0 ]; do
    actual="${pendientes[0]}"
    pendientes=("${pendientes[@]:1}")
    analizar "$actual"
done

if [ "$copiadas" -eq 0 ]; then
    echo "Error: no se copió ninguna DLL desde $PREFIX/bin; la carpeta no funcionará." >&2
    exit 1
fi

# --- 4. Licencias de las librerías incluidas ---------------------------------
# Para cada DLL copiada se busca con pacman el paquete de MSYS2 del que procede y
# se copian sus archivos de licencia. PAQUETES.txt resume paquete, versión y
# licencia, para revisar qué obligaciones conlleva distribuir la carpeta.
inventario="$DIST/licenses/PAQUETES.txt"
{
    echo "Paquetes de MSYS2 incluidos en esta carpeta (paquete, versión, licencia)"
    echo "Generado por empaquetar_windows.sh"
    echo
} > "$inventario"
if command -v pacman >/dev/null 2>&1; then
    declare -A paquetes=()
    for dll in "$DIST/bin/"*.dll; do
        nombre="$(basename "$dll")"
        [ -f "$PREFIX/bin/$nombre" ] || continue        # libcvcore.dll es del propio proyecto
        paquete="$(pacman -Qqo "$PREFIX/bin/$nombre" 2>/dev/null | head -n 1 || true)"
        [ -n "$paquete" ] && paquetes[$paquete]=1
    done
    for paquete in $(printf '%s\n' "${!paquetes[@]}" | sort); do
        version="$(pacman -Q "$paquete" 2>/dev/null | awk '{ print $2 }' || true)"
        licencia="$(pacman -Qi "$paquete" 2>/dev/null | sed -n 's/^Licen[sc]es* *: *//p' | head -n 1 || true)"
        printf '%-50s %-20s %s\n' "$paquete" "$version" "$licencia" >> "$inventario"
        if [ -d "$PREFIX/share/licenses/$paquete" ]; then
            cp -r "$PREFIX/share/licenses/$paquete" "$DIST/licenses/"
        fi
    done
    echo "Licencias:   $DIST/licenses (resumen en PAQUETES.txt)"
else
    echo "Aviso: no se encuentra pacman; no se han copiado las licencias de las librerías." >&2
    echo "(no se pudo generar: falta pacman)" >> "$inventario"
fi

echo
echo "DLL copiadas: $copiadas"
echo "Tamaño total: $(du -sh "$DIST" | cut -f1)"
echo "Listo. Ejecutable: $DIST/bin/lulumoy.exe"

# --- 5. Instalador con NSIS (opcional) --------------------------------------
if [ "$INSTALADOR" -eq 1 ]; then
    echo
    echo "Creando el instalador con NSIS..."
    # Se prefiere el NSIS oficial (https://nsis.sourceforge.io): el paquete de
    # MSYS2 3.13 tiene un fallo conocido ("Plugin not found") con los
    # instaladores que genera CPack.
    nsis=""
    for candidato in "/c/Program Files (x86)/NSIS/makensis.exe" "/c/Program Files/NSIS/makensis.exe"; do
        if [ -f "$candidato" ]; then nsis="$candidato"; break; fi
    done
    if [ -z "$nsis" ] && command -v makensis >/dev/null 2>&1; then
        nsis="$(command -v makensis)"
    fi
    if [ -z "$nsis" ]; then
        echo "Error: no se encuentra NSIS. Instálelo desde https://nsis.sourceforge.io" >&2
        echo "       (o con: pacman -S mingw-w64-ucrt-x86_64-nsis) y repita." >&2
        exit 1
    fi
    echo "NSIS:     $nsis"

    if ( cd "$BUILD" \
         && cmake -DLULUMOY_DIST_DIR="$(cygpath -m "$DIST")" . >/dev/null \
         && cpack -G NSIS -D CPACK_NSIS_EXECUTABLE="$(cygpath -m "$nsis")" ); then
        echo "Instalador: $ROOT/instalador/"
    else
        echo >&2
        echo "Error al crear el instalador. Si el mensaje dice \"Plugin not found\", es el fallo" >&2
        echo "conocido del NSIS de MSYS2: instale el NSIS oficial desde https://nsis.sourceforge.io" >&2
        echo "y vuelva a ejecutar: ./empaquetar_windows.sh --instalador" >&2
        exit 1
    fi
fi
