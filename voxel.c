#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

void fail() {
    fputs("FAILURE", stderr);
    exit(1);
}

void check(bool assertion) {
    if (!assertion) {
        fail();
    }
}

typedef struct {
    int height;
    int width;
    unsigned char (**data)[3];
} Image;

void init_image(Image *image, int height, int width) {
    image->height = height;
    image->width = width;
    image->data = malloc((size_t)height * sizeof(*image->data));
    check(image->data != NULL);
    for (int i = 0; i < height; ++i) {
        image->data[i] = malloc((size_t)width * sizeof(**image->data));
        check(image->data[i] != NULL);
    }
}

void free_image(Image *image) {
    for (int i = 0; i < image->height; ++i) {
        free(image->data[i]);
    }
    free(image->data);
    image->height = 0;
    image->width = 0;
    image->data = NULL;
}

void save_image(const Image *image, const char *filename) {
    FILE *f = fopen(filename, "wb");
    check(f != NULL);
    fprintf(f, "P6\n%d %d\n255\n", image->width, image->height);
    for (int i = 0; i < image->height; ++i) {
        fwrite(image->data[i], (size_t)image->width, sizeof(**image->data), f);
    }
    fclose(f);
}

void help() {
    puts("Commands:\n");
    puts("help            shows help");
    puts("test-ppm        renders sample of ppm image");
    puts("test-gtk        opens GTK window");
}

void test_ppm() {
    Image image;
    init_image(&image, 480, 640);
    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            image.data[i][j][0] = 255;
            image.data[i][j][1] = 255;
            image.data[i][j][2] = 255;
        }
    }
    for (int i = 100; i < image.height - 50; ++i) {
        for (int j = 200; j < image.width - 100; ++j) {
            image.data[i][j][0] = 255;
            image.data[i][j][1] = 0;
            image.data[i][j][2] = 0;
        }
    }
    save_image(&image, "voxel-c.ppm");
    free_image(&image);
}

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

typedef struct {
    double a;
    double b;
} Range;

bool is_empty_range(const Range *r) { return r->a > r->b; }

Range intersectRanges(const Range *r1, const Range *r2) {
    Range r = { fmax(r1->a, r2->a), fmin(r1->b, r2->b) };
    return r;
}

typedef struct {
    double x;
    double y;
    double z;
} V3;

typedef struct {
    V3 p;
    V3 q;
} Ray;

double getParameter(double a, double b, double x) {
    return (x - a) / (b - a);
}

Range getParameterRange(double a, double b) {
    double p1 = getParameter(a, b, -0.5);
    double p2 = getParameter(a, b,  0.5);
    Range r = {fmin(p1, p2), fmax(p1, p2)};
    return r;
}

Range intersectUnitCube(const Ray *r) {
    Range r_ = { 0, INFINITY };

    Range rx = getParameterRange(r->p.x, r->q.x);
    r_ = intersectRanges(&r_, &rx);

    Range ry = getParameterRange(r->p.y, r->q.y);
    r_ = intersectRanges(&r_, &ry);

    Range rz = getParameterRange(r->p.z, r->q.z);
    r_ = intersectRanges(&r_, &rz);

    return r_;
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
    const Ray axis = {{0, 0, -50}, {0, 0, 0}};
    const double px = 30. / 1980.;

    WindowData *data = (WindowData *)data_;
    cairo_surface_t *surface = data->surface;

    cairo_surface_flush(surface);
    cairo_format_t format = cairo_image_surface_get_format(surface);
    if (format == CAIRO_FORMAT_ARGB32) {
        unsigned char *image_data = cairo_image_surface_get_data(surface);
        const int stride = cairo_image_surface_get_stride(surface);
        for (int y = 0; y < height; ++y) {
            uint32_t *row = (void *)image_data;
            for (int x = 0; x < width; ++x) {
                Ray ray = {
                    axis.p,
                    {
                        px * (x - (width - 1.) / 2),
                        px * ((height - 1.) / 2 - y),
                        axis.q.z
                    }
                };
                uint32_t r;
                uint32_t g;
                uint32_t b;
                Range range = intersectUnitCube(&ray);
                if (is_empty_range(&range)) {
                    r = g = b = 0;
                } else {
                    r = g = b = 255;
                }
                row[x] = 255U << 24 | r << 16 | g << 8 | b;
            }
            image_data += stride;
        }
    }
    cairo_surface_mark_dirty(surface);

    cairo_set_source_surface(cr, surface, 0, 0);
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

int test_gtk(int argc, char *argv[]) {
    WindowData data = {0};

    GtkApplication *app = gtk_application_new(
        "ru.slavav.voxel-c",
        G_APPLICATION_DEFAULT_FLAGS
    );
    g_signal_connect(app, "activate", G_CALLBACK(activate), &data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        help();
    }
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "help") == 0) {
            help();
        } else if (strcmp(argv[i], "test-ppm") == 0) {
            test_ppm();
        } else if (strcmp(argv[i], "test-gtk") == 0) {
            return test_gtk(1, argv);
        } else {
            printf("What is %s\n", argv[i]);
            help();
            exit(1);
        }
    }
}
