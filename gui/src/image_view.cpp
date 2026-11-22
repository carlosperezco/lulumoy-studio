/**
 * @file image_view.cpp
 * @brief Implementación del visor con reglas en píxeles.
 */
#include "image_view.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include "cvcore/image_io.h"

namespace cvgui {

namespace {

const int kRuler = 22;            ///< Grosor de las reglas (px de pantalla).
const double kLabelSpacing = 60.0; ///< Separación mínima entre números de la regla.
const double kMinorSpacing = 5.0;  ///< Separación mínima entre marcas pequeñas.
const double kGridZoom = 8.0;      ///< Zoom a partir del cual se dibuja la rejilla de píxeles.
const double kPi = 3.14159265358979323846;

/// Paso entre números: 1, 2, 5, 10, 20, 50... el menor que deja kLabelSpacing en pantalla.
long majorStep(double zoom) {
    for (long power = 1;; power *= 10) {
        for (long base : {1L, 2L, 5L}) {
            const long step = base * power;
            if (step * zoom >= kLabelSpacing || step >= 100000000L) return step;
        }
    }
}

/// Paso entre marcas pequeñas: un divisor del paso principal, o 0 si no caben.
long minorStep(long major, double zoom) {
    for (long divisions : {10L, 5L, 2L}) {
        if (major % divisions != 0) continue;
        const long step = major / divisions;
        if (step * zoom >= kMinorSpacing) return step;
    }
    return 0;
}

} // namespace

ImageView::ImageView() {
    grid_ = gtk_grid_new();

    corner_ = gtk_drawing_area_new();
    gtk_widget_set_size_request(corner_, kRuler, kRuler);
    top_ = gtk_drawing_area_new();
    gtk_widget_set_size_request(top_, -1, kRuler);
    gtk_widget_set_hexpand(top_, TRUE);
    left_ = gtk_drawing_area_new();
    gtk_widget_set_size_request(left_, kRuler, -1);
    gtk_widget_set_vexpand(left_, TRUE);

    scrolled_ = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_hexpand(scrolled_, TRUE);
    gtk_widget_set_vexpand(scrolled_, TRUE);
    image_ = gtk_image_new();
    // La imagen va dentro de una caja de eventos centrada y ajustada a su
    // tamaño: así la posición del ratón, dividida por el zoom, es
    // directamente la coordenada en píxeles de la imagen.
    eventBox_ = gtk_event_box_new();
    gtk_widget_set_halign(eventBox_, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(eventBox_, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(eventBox_), image_);
    gtk_widget_add_events(eventBox_, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
    gtk_container_add(GTK_CONTAINER(scrolled_), eventBox_);

    gtk_grid_attach(GTK_GRID(grid_), corner_, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_), top_, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_), left_, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_), scrolled_, 1, 1, 1, 1);

    g_signal_connect(top_, "draw", G_CALLBACK(&ImageView::onRulerDraw), this);
    g_signal_connect(left_, "draw", G_CALLBACK(&ImageView::onRulerDraw), this);
    g_signal_connect(corner_, "draw", G_CALLBACK(&ImageView::onCornerDraw), this);
    g_signal_connect_after(image_, "draw", G_CALLBACK(&ImageView::onCanvasDraw), this);
    g_signal_connect(eventBox_, "motion-notify-event", G_CALLBACK(&ImageView::onMotion), this);
    g_signal_connect(eventBox_, "leave-notify-event", G_CALLBACK(&ImageView::onLeave), this);
    g_signal_connect(eventBox_, "size-allocate", G_CALLBACK(&ImageView::onAllocate), this);
    GtkAdjustment* h = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_));
    GtkAdjustment* v = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_));
    g_signal_connect(h, "value-changed", G_CALLBACK(&ImageView::onScroll), this);
    g_signal_connect(v, "value-changed", G_CALLBACK(&ImageView::onScroll), this);
}

void ImageView::setImage(const cv::Mat& image, double zoom) {
    zoom_ = zoom > 0.0 ? zoom : 1.0;
    if (image.empty()) {
        width_ = height_ = 0;
        pointerInside_ = false;
        gtk_image_clear(GTK_IMAGE(image_));
        redrawRulers();
        return;
    }
    width_ = image.cols;
    height_ = image.rows;

    const cv::Mat rgb = cvcore::toDisplayRgb(image);
    GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, rgb.cols, rgb.rows);
    if (!pixbuf) return;
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    const int stride = gdk_pixbuf_get_rowstride(pixbuf);
    for (int y = 0; y < rgb.rows; ++y) {
        std::memcpy(pixels + static_cast<std::size_t>(y) * stride, rgb.ptr(y), static_cast<std::size_t>(rgb.cols) * 3);
    }
    const int w = std::max(1, static_cast<int>(std::lround(rgb.cols * zoom_)));
    const int h = std::max(1, static_cast<int>(std::lround(rgb.rows * zoom_)));
    if (w != rgb.cols || h != rgb.rows) {
        GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, w, h, zoom_ < 1.0 ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST);
        g_object_unref(pixbuf);
        pixbuf = scaled;
    }
    if (pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_), pixbuf);
        g_object_unref(pixbuf);
    }
    redrawRulers();
}

void ImageView::redrawRulers() {
    gtk_widget_queue_draw(top_);
    gtk_widget_queue_draw(left_);
}

/**
 * @brief Dibuja una regla.
 *
 * La posición del origen de la imagen dentro de la regla se obtiene
 * traduciendo la esquina de la caja de eventos a las coordenadas de la
 * regla, así que incluye el desplazamiento y el centrado de la imagen.
 */
void ImageView::drawRuler(GtkWidget* ruler, cairo_t* cr, bool horizontal) {
    const int length = horizontal ? gtk_widget_get_allocated_width(ruler) : gtk_widget_get_allocated_height(ruler);
    const int thick = horizontal ? gtk_widget_get_allocated_height(ruler) : gtk_widget_get_allocated_width(ruler);

    // Fondo y línea que separa la regla de la imagen.
    cairo_set_source_rgb(cr, 0.86, 0.87, 0.86);
    cairo_paint(cr);
    if (width_ <= 0 || height_ <= 0) return;

    gint ox = 0, oy = 0;
    if (!gtk_widget_translate_coordinates(eventBox_, ruler, 0, 0, &ox, &oy)) return;
    const double origin = horizontal ? ox : oy;
    const int extent = horizontal ? width_ : height_;

    // Zona que ocupa la imagen, más clara: fuera de ella no hay píxeles.
    cairo_set_source_rgb(cr, 0.97, 0.97, 0.96);
    if (horizontal) cairo_rectangle(cr, origin, 0, extent * zoom_, thick);
    else cairo_rectangle(cr, 0, origin, thick, extent * zoom_);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.55, 0.56, 0.55);
    cairo_set_line_width(cr, 1.0);
    if (horizontal) { cairo_move_to(cr, 0, thick - 0.5); cairo_line_to(cr, length, thick - 0.5); }
    else { cairo_move_to(cr, thick - 0.5, 0); cairo_line_to(cr, thick - 0.5, length); }
    cairo_stroke(cr);

    // Marcas visibles, solo dentro de la imagen (0 ... extent).
    const long major = majorStep(zoom_);
    const long minor = minorStep(major, zoom_);
    const long tick = minor > 0 ? minor : major;
    const long first = std::max(0L, static_cast<long>(std::floor(-origin / zoom_)));
    const long last = std::min(static_cast<long>(extent), static_cast<long>(std::ceil((length - origin) / zoom_)));

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 9.5);
    cairo_set_source_rgb(cr, 0.15, 0.16, 0.15);
    for (long v = (first + tick - 1) / tick * tick; v <= last; v += tick) {
        const double pos = std::floor(origin + v * zoom_) + 0.5;
        const bool isMajor = v % major == 0;
        const double size = isMajor ? thick * 0.55 : thick * 0.22;
        if (horizontal) { cairo_move_to(cr, pos, thick); cairo_line_to(cr, pos, thick - size); }
        else { cairo_move_to(cr, thick, pos); cairo_line_to(cr, thick - size, pos); }
        cairo_stroke(cr);
        if (!isMajor) continue;

        const std::string text = std::to_string(v);
        cairo_text_extents_t te;
        cairo_text_extents(cr, text.c_str(), &te);
        if (horizontal) {
            cairo_move_to(cr, pos + 3.0, 10.0);
            cairo_show_text(cr, text.c_str());
        } else { // texto girado, leído de abajo arriba, a continuación de la marca
            cairo_save(cr);
            cairo_move_to(cr, 10.0, pos + 3.0 + te.x_advance);
            cairo_rotate(cr, -kPi / 2.0);
            cairo_show_text(cr, text.c_str());
            cairo_restore(cr);
        }
    }

    // Posición del ratón.
    if (pointerInside_) {
        const double p = std::floor(origin + (horizontal ? pointerX_ : pointerY_) * zoom_) + 0.5;
        cairo_set_source_rgb(cr, 0.85, 0.1, 0.1);
        cairo_set_line_width(cr, 1.5);
        if (horizontal) { cairo_move_to(cr, p, 0); cairo_line_to(cr, p, thick); }
        else { cairo_move_to(cr, 0, p); cairo_line_to(cr, thick, p); }
        cairo_stroke(cr);
    }
}

/// Esquina: los dos ejes y su sentido.
void ImageView::drawCorner(cairo_t* cr) {
    cairo_set_source_rgb(cr, 0.86, 0.87, 0.86);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.15, 0.16, 0.15);
    cairo_set_line_width(cr, 1.2);
    const double o = 5.5, end = kRuler - 3.5;
    cairo_move_to(cr, o, o); cairo_line_to(cr, end, o);   // x →
    cairo_move_to(cr, o, o); cairo_line_to(cr, o, end);   // y ↓
    cairo_stroke(cr);
    cairo_move_to(cr, end, o); cairo_line_to(cr, end - 3.5, o - 2.5); cairo_line_to(cr, end - 3.5, o + 2.5); cairo_close_path(cr);
    cairo_move_to(cr, o, end); cairo_line_to(cr, o - 2.5, end - 3.5); cairo_line_to(cr, o + 2.5, end - 3.5); cairo_close_path(cr);
    cairo_fill(cr);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 8.5);
    cairo_move_to(cr, 10.0, 15.0); cairo_show_text(cr, "x");
    cairo_move_to(cr, 1.0, kRuler - 0.5); cairo_show_text(cr, "y");
}

/// Rejilla de píxeles con zoom grande (solo en la zona visible).
void ImageView::drawPixelGrid(cairo_t* cr) {
    if (zoom_ < kGridZoom || width_ <= 0) return;
    double x0, y0, x1, y1;
    cairo_clip_extents(cr, &x0, &y0, &x1, &y1);
    const int c0 = std::max(0, static_cast<int>(x0 / zoom_));
    const int c1 = std::min(width_, static_cast<int>(x1 / zoom_) + 1);
    const int r0 = std::max(0, static_cast<int>(y0 / zoom_));
    const int r1 = std::min(height_, static_cast<int>(y1 / zoom_) + 1);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 0.45);
    for (int c = c0; c <= c1; ++c) {
        const double x = std::floor(c * zoom_) + 0.5;
        cairo_move_to(cr, x, r0 * zoom_);
        cairo_line_to(cr, x, r1 * zoom_);
    }
    for (int r = r0; r <= r1; ++r) {
        const double y = std::floor(r * zoom_) + 0.5;
        cairo_move_to(cr, c0 * zoom_, y);
        cairo_line_to(cr, c1 * zoom_, y);
    }
    cairo_stroke(cr);
}

gboolean ImageView::onRulerDraw(GtkWidget* widget, cairo_t* cr, gpointer self) {
    auto* view = static_cast<ImageView*>(self);
    view->drawRuler(widget, cr, widget == view->top_);
    return TRUE;
}

gboolean ImageView::onCornerDraw(GtkWidget*, cairo_t* cr, gpointer self) {
    static_cast<ImageView*>(self)->drawCorner(cr);
    return TRUE;
}

gboolean ImageView::onCanvasDraw(GtkWidget*, cairo_t* cr, gpointer self) {
    static_cast<ImageView*>(self)->drawPixelGrid(cr);
    return FALSE;
}

gboolean ImageView::onMotion(GtkWidget*, GdkEvent* event, gpointer self) {
    auto* view = static_cast<ImageView*>(self);
    gdouble x = 0.0, y = 0.0;
    if (view->width_ <= 0 || !gdk_event_get_coords(event, &x, &y)) return FALSE;
    const double px = x / view->zoom_;
    const double py = y / view->zoom_;
    view->pointerInside_ = px >= 0.0 && py >= 0.0 && px < view->width_ && py < view->height_;
    view->pointerX_ = px;
    view->pointerY_ = py;
    view->redrawRulers();
    if (view->onPointer_) view->onPointer_(view->pointerInside_, px, py);
    return FALSE;
}

gboolean ImageView::onLeave(GtkWidget*, GdkEvent*, gpointer self) {
    auto* view = static_cast<ImageView*>(self);
    view->pointerInside_ = false;
    view->redrawRulers();
    if (view->onPointer_) view->onPointer_(false, 0.0, 0.0);
    return FALSE;
}

void ImageView::onScroll(GtkAdjustment*, gpointer self) {
    static_cast<ImageView*>(self)->redrawRulers();
}

std::string pixelText(const cv::Mat& image, double x, double y, const std::string& level) {
    if (image.empty() || x < 0.0 || y < 0.0) return {};
    const int col = static_cast<int>(std::floor(x));
    const int row = static_cast<int>(std::floor(y));
    if (col >= image.cols || row >= image.rows) return {};

    auto value = [&](int channel) {
        return image.depth() == CV_16U ? static_cast<int>(image.ptr<ushort>(row)[col * image.channels() + channel])
                                       : static_cast<int>(image.ptr<uchar>(row)[col * image.channels() + channel]);
    };
    std::string text = "(x, y) = (" + std::to_string(col) + ", " + std::to_string(row) + ")  \xC2\xB7  ";
    if (image.channels() == 1) return text + level + " = " + std::to_string(value(0));
    // OpenCV guarda los canales como B, G, R (y A).
    text += image.channels() == 4 ? "RGBA = (" : "RGB = (";
    text += std::to_string(value(2)) + ", " + std::to_string(value(1)) + ", " + std::to_string(value(0));
    if (image.channels() == 4) text += ", " + std::to_string(value(3));
    return text + ")";
}

void ImageView::onAllocate(GtkWidget*, GdkRectangle*, gpointer self) {
    static_cast<ImageView*>(self)->redrawRulers();
}

} // namespace cvgui
