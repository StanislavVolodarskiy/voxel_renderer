#include <gtk/gtk.h>

typedef struct {
    cairo_surface_t *surface;
    double           start_x;
    double           start_y;
    GtkWidget       *area;
} WindowData;

static void clear_surface(WindowData *data) {
    cairo_t *cr = cairo_create(data->surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static void resize_cb(
    GtkWidget  *widget,
    int         width,
    int         height,
    WindowData *data
) {
    if (data->surface) {
        cairo_surface_destroy(data->surface);
        data->surface = NULL;
    }

    if (gtk_native_get_surface(gtk_widget_get_native(widget))) {
        data->surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            gtk_widget_get_width(widget),
            gtk_widget_get_height(widget));

        /* Initialize the surface to white */
        clear_surface(data);
    }
}

/* Redraw the screen from the surface. Note that the draw
 * callback receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static void draw_cb(
    GtkDrawingArea *drawing_area,
    cairo_t        *cr,
    int             width,
    int             height,
    gpointer        data_
) {
    WindowData *data = data_;
    cairo_set_source_surface(cr, data->surface, 0, 0);
    cairo_paint(cr);
}

/* Draw a rectangle on the surface at the given position */
static void draw_brush(double x, double y, WindowData *data) {
    cairo_t *cr;

    /* Paint to the surface, where we store our state */
    cr = cairo_create(data->surface);

    cairo_rectangle(cr, x - 3, y - 3, 6, 6);
    cairo_fill(cr);

    cairo_destroy(cr);

    /* Now invalidate the drawing area. */
    gtk_widget_queue_draw(data->area);
}

static void drag_begin(
    GtkGestureDrag *gesture,
    double          x,
    double          y,
    WindowData     *data
) {
    data->start_x = x;
    data->start_y = y;
    draw_brush(x, y, data);
}

static void drag_update(
    GtkGestureDrag *gesture,
    double          x,
    double          y,
    WindowData     *data
) {
    draw_brush(data->start_x + x, data->start_y + y, data);
}

static void drag_end(
    GtkGestureDrag *gesture,
    double          x,
    double          y,
    WindowData     *data
) {
    draw_brush(data->start_x + x, data->start_y + y, data);
}

static void pressed(
    GtkGestureClick *gesture,
    int              n_press,
    double           x,
    double           y,
    WindowData      *data
) {
    clear_surface(data);
    gtk_widget_queue_draw(data->area);
}

static void close_window(WindowData *data) {
    if (data->surface) {
        cairo_surface_destroy(data->surface);
    }
}

static void activate(GtkApplication *app, WindowData *data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");

    g_signal_connect(window, "destroy", G_CALLBACK(close_window), data);

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_window_set_child(GTK_WINDOW(window), frame);

    data->area = gtk_drawing_area_new();
    /* set a minimum size */
    gtk_widget_set_size_request(data->area, 640, 480);

    gtk_frame_set_child(GTK_FRAME(frame), data->area);

    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(data->area), 
        draw_cb, 
        data, 
        NULL
    );

    g_signal_connect_after(data->area, "resize", G_CALLBACK(resize_cb), data);


    GtkGesture *drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(data->area, GTK_EVENT_CONTROLLER(drag));
    g_signal_connect(drag, "drag-begin", G_CALLBACK(drag_begin), data);
    g_signal_connect(drag, "drag-update", G_CALLBACK(drag_update), data);
    g_signal_connect(drag, "drag-end", G_CALLBACK(drag_end), data);

    GtkGesture *press = gtk_gesture_click_new();
    gtk_gesture_single_set_button(
        GTK_GESTURE_SINGLE(press),
        GDK_BUTTON_SECONDARY
    );
    gtk_widget_add_controller(data->area, GTK_EVENT_CONTROLLER(press));

    g_signal_connect(press, "pressed", G_CALLBACK(pressed), data);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    WindowData data = {0};

    GtkApplication *app = gtk_application_new(
        "org.gtk.example",
        G_APPLICATION_DEFAULT_FLAGS
    );
    g_signal_connect(app, "activate", G_CALLBACK(activate), &data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

