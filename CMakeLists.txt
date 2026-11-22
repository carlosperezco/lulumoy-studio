cmake_minimum_required(VERSION 3.16)
# Versión de Lulumoy Studio (MAYOR.MENOR.PARCHE). Es el único lugar donde se cambia:
# de aquí salen el "Acerca de", "lulumoy --version", el nombre del instalador,
# las propiedades del .exe y la documentación. Ver "Versiones" en el README.
project(lulumoy VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Opciones generales
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Todo (ejecutable y librerías) queda junto en build/bin.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# cvcore es una librería dinámica; en Windows se exportan todos sus símbolos.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# En Linux el ejecutable busca libcvcore.so en su propia carpeta.
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib")

if(MSVC)
    add_compile_options(/utf-8 /W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Dependencias: solo OpenCV y GTK 3
# ---------------------------------------------------------------------------
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

# ---------------------------------------------------------------------------
# cvcore: operaciones (OpenCV, sin interfaz gráfica)
#
# Cada .cpp de core/modules es un módulo que se registra solo. Para añadir
# una operación basta con crear un archivo ahí; CMake lo detecta.
# Es una librería dinámica para que el enlazador no descarte los módulos.
# ---------------------------------------------------------------------------
file(GLOB CVCORE_MODULES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/core/modules/*.cpp)

add_library(cvcore SHARED
    core/src/image_info.cpp
    core/src/image_io.cpp
    core/src/params.cpp
    core/src/pipeline.cpp
    core/src/registry.cpp
    core/src/vectorize.cpp
    ${CVCORE_MODULES}
)
target_include_directories(cvcore PUBLIC core/include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cvcore PUBLIC ${OpenCV_LIBS})

# ---------------------------------------------------------------------------
# cvgui: interfaz gráfica (GTK 3)
# ---------------------------------------------------------------------------
add_library(cvgui STATIC
    gui/src/app_config.cpp
    gui/src/help.cpp
    gui/src/i18n.cpp
    gui/src/kvfile.cpp
    gui/src/main_window.cpp
    gui/src/param_dialog.cpp
    gui/src/image_view.cpp
    gui/src/param_editor.cpp
    gui/src/properties_dialog.cpp
    gui/src/vectorize_dialog.cpp
)
# Cabecera con la versión, generada en build/generated.
configure_file(app/version.h.in ${CMAKE_BINARY_DIR}/generated/lulumoy_version.h @ONLY)
target_include_directories(cvgui PUBLIC gui/include ${CMAKE_BINARY_DIR}/generated PRIVATE gui/src)
target_link_libraries(cvgui PUBLIC cvcore PRIVATE PkgConfig::GTK3)

# ---------------------------------------------------------------------------
# lulumoy: ejecutable (gestión central)
# ---------------------------------------------------------------------------
add_executable(lulumoy app/main.cpp)
target_link_libraries(lulumoy PRIVATE cvgui cvcore)

# Windows: icono y datos de versión dentro del .exe.
if(WIN32)
    enable_language(RC)
    set(LULUMOY_ICON_FILE "${CMAKE_SOURCE_DIR}/assets/lulumoy.ico")
    configure_file(app/lulumoy.rc.in ${CMAKE_BINARY_DIR}/generated/lulumoy.rc @ONLY)
    target_sources(lulumoy PRIVATE ${CMAKE_BINARY_DIR}/generated/lulumoy.rc)
endif()

# En Windows (MinGW) oculta la consola. Desactívelo para ver la salida de
# "lulumoy --list" y "lulumoy --apply".
option(LULUMOY_HIDE_CONSOLE "Ocultar la consola en Windows" ON)
if(WIN32 AND MINGW AND LULUMOY_HIDE_CONSOLE)
    target_link_options(lulumoy PRIVATE -mwindows)
endif()

# Copia configuración, idiomas e imágenes junto al ejecutable.
add_custom_command(TARGET lulumoy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:lulumoy>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/lang $<TARGET_FILE_DIR:lulumoy>/lang
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:lulumoy>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/help $<TARGET_FILE_DIR:lulumoy>/help
    COMMENT "Copiando recursos (config, lang, assets, help)"
)

# ---------------------------------------------------------------------------
# Instalación
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
if(WIN32)
    # En Windows se instala tal cual la carpeta dist/ que crea
    # empaquetar_windows.sh: programa, DLL, recursos de GTK y licencias.
    set(LULUMOY_DIST_DIR "${CMAKE_SOURCE_DIR}/dist" CACHE PATH
        "Carpeta creada por empaquetar_windows.sh")
    install(CODE "
        if(NOT EXISTS \"${LULUMOY_DIST_DIR}/bin/lulumoy.exe\")
            message(FATAL_ERROR \"No existe ${LULUMOY_DIST_DIR}/bin/lulumoy.exe. Ejecute primero ./empaquetar_windows.sh\")
        endif()")
    install(DIRECTORY "${LULUMOY_DIST_DIR}/" DESTINATION .)
else()
    install(TARGETS lulumoy cvcore
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY config lang assets help DESTINATION ${CMAKE_INSTALL_DATADIR}/lulumoy)
endif()

# ---------------------------------------------------------------------------
# Instalador de Windows con NSIS: "./empaquetar_windows.sh --instalador"
# (o, con dist/ ya creada: "cd build && cpack -G NSIS")
# ---------------------------------------------------------------------------
if(WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY)

    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "Lulumoy Studio")
    set(CPACK_PACKAGE_VENDOR "DonatellaTech")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Editor de imágenes modular con GTK y OpenCV")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://donatellatech.com")
    set(CPACK_PACKAGE_FILE_NAME "LulumoyStudio-${PROJECT_VERSION}-win64")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/instalador")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")

    # Carpeta propuesta: C:\Program Files\DonatellaTech\Lulumoy Studio
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "DonatellaTech\\\\Lulumoy Studio")
    # Clave del registro sin subcarpetas: si no, Windows no muestra el programa
    # en "Aplicaciones instaladas" y no se puede desinstalar desde ahí.
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Lulumoy Studio")
    set(CPACK_NSIS_DISPLAY_NAME "Lulumoy Studio")
    set(CPACK_NSIS_PACKAGE_NAME "Lulumoy Studio")
    set(CPACK_NSIS_BRANDING_TEXT "DonatellaTech")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://donatellatech.com")

    # Acceso directo en el menú Inicio (y, si se marca, en el escritorio),
    # icono en "Aplicaciones instaladas" y opción de abrir el programa al terminar.
    set(CPACK_PACKAGE_EXECUTABLES "lulumoy" "Lulumoy Studio")
    set(CPACK_CREATE_DESKTOP_LINKS "lulumoy")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\lulumoy.exe")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "lulumoy.exe")

    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/lulumoy.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/lulumoy.ico")

    include(CPack)
endif()

# ---------------------------------------------------------------------------
# Documentación: "cmake --build build --target docs" (necesita Doxygen)
# ---------------------------------------------------------------------------
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E env LULUMOY_VERSION=${PROJECT_VERSION}
                ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generando documentación en docs/html"
        VERBATIM
    )
endif()
