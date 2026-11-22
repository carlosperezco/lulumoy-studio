/**
 * @file image_view.h
 * @brief Visor de imagen con zoom, barras de desplazamiento y reglas en píxeles.
 */
#pragma once

#include <functional>
#include <string>

#include <gtk/gtk.h>

#include <opencv2/core.hpp>

namespace cvgui {

/**
 * @brief Muestra una imagen con reglas graduadas en píxeles alrededor.
 *
 * Las reglas (arriba el eje x, a la izquierda el eje y) quedan fuera de la
 * imagen y siguen al desplazamiento y al zoom. El origen (0, 0) es la esquina
 * superior izquierda; x crece hacia la derecha e y hacia abajo, como en
 * OpenCV. La coordenada x corresponde al borde izquierdo del píxel x, así que
 * el píxel (x, y) ocupa el cuadrado [x, x+1) × [y, y+1). Con zoom de 800 % o
 * más se dibuja además la rejilla de píxeles.
 *
 * Al mover el ratón sobre la imagen se marca su posición en las reglas y se
 * avisa con la función de setOnPointer(). La posición de un clic en
 * eventBox(), dividida por zoom(), es la coordenada en la imagen.
 *
 * El objeto debe vivir mientras exista su widget. No se puede copiar.
 */
class ImageView {
public:
    ImageView();
    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    /// Widget completo (reglas + zona desplazable), para insertarlo en una ventana.
    GtkWidget* widget() const { return grid_; }
    /// Zona desplazable (para fijar su tamaño mínimo).
    GtkWidget* scrolled() const { return scrolled_; }
    /// Imagen mostrada; se puede conectar a su señal "draw" (con g_signal_connect_after) para dibujar encima.
    GtkWidget* canvas() const { return image_; }
    /// Caja que recibe los clics; tiene exactamente el tamaño de la imagen mostrada.
    GtkWidget* eventBox() const { return eventBox_; }

    /**
     * @brief Muestra una imagen (8 o 16 bits, 1, 3 o 4 canales).
     * @param image Imagen; vacía para borrar el visor.
     * @param zoom  Escala de pantalla (1 = 100 %).
     */
    void setImage(const cv::Mat& image, double zoom);
    /// Borra la imagen.
    void clear() { setImage(cv::Mat(), 1.0); }

    double zoom() const { return zoom_; }
    int imageWidth() const { return width_; }
    int imageHeight() const { return height_; }

    /**
     * @brief Función que se llama al mover el ratón sobre la imagen o al salir de ella.
     *
     * Recibe si el puntero está dentro y su posición en coordenadas de la
     * imagen (con decimales).
     */
    void setOnPointer(std::function<void(bool, double, double)> callback) { onPointer_ = std::move(callback); }

private:
    void redrawRulers();
    void drawRuler(GtkWidget* ruler, cairo_t* cr, bool horizontal);
    void drawCorner(cairo_t* cr);
    void drawPixelGrid(cairo_t* cr);

    static gboolean onRulerDraw(GtkWidget* widget, cairo_t* cr, gpointer self);
    static gboolean onCornerDraw(GtkWidget* widget, cairo_t* cr, gpointer self);
    static gboolean onCanvasDraw(GtkWidget* widget, cairo_t* cr, gpointer self);
    static gboolean onMotion(GtkWidget* widget, GdkEvent* event, gpointer self);
    static gboolean onLeave(GtkWidget* widget, GdkEvent* event, gpointer self);
    static void onScroll(GtkAdjustment* adjustment, gpointer self);
    static void onAllocate(GtkWidget* widget, GdkRectangle* allocation, gpointer self);

    GtkWidget* grid_ = nullptr;
    GtkWidget* corner_ = nullptr;
    GtkWidget* top_ = nullptr;   ///< Regla del eje x.
    GtkWidget* left_ = nullptr;  ///< Regla del eje y.
    GtkWidget* scrolled_ = nullptr;
    GtkWidget* eventBox_ = nullptr;
    GtkWidget* image_ = nullptr;

    int width_ = 0;  ///< Tamaño de la imagen original (no de la mostrada).
    int height_ = 0;
    double zoom_ = 1.0;
    bool pointerInside_ = false;
    double pointerX_ = 0.0;
    double pointerY_ = 0.0;
    std::function<void(bool, double, double)> onPointer_;
};

/**
 * @brief Texto con la posición y el valor de un píxel, p. ej. "(x, y) = (12, 40) · RGB = (200, 31, 0)".
 * @param image Imagen (BGR, BGRA o un canal; 8 o 16 bits).
 * @param x,y   Posición en coordenadas de la imagen; se toma el píxel que la contiene.
 * @param level Palabra para el valor de un solo canal ("nivel").
 * @return Texto vacío si la posición está fuera de la imagen.
 */
std::string pixelText(const cv::Mat& image, double x, double y, const std::string& level);

} // namespace cvgui
