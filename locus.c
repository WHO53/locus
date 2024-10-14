#include "locus.h"
#include <cairo/cairo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

static void handle_ping(void *data, struct xdg_wm_base *xdg_wm_base,
        uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = handle_ping,
};

static void create_buffer(Locus *app);

static void handle_configure(void *data, struct xdg_surface *xdg_surface,
        uint32_t serial) {
    Locus *app = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    app->configured = 1;
    if (app->buffer) {
        app->draw_callback(app->cr, app->width, app->height);
        cairo_surface_flush(app->cairo_surface);
        wl_surface_attach(app->surface, app->buffer, 0, 0);
        wl_surface_damage(app->surface, 0, 0, app->width, app->height);
        wl_surface_commit(app->surface);
    }
}

static void handle_xdg_toplevel_configure(void *data,
        struct xdg_toplevel *xdg_toplevel,
        int32_t width, int32_t height,
        struct wl_array *states) {
}

static void handle_xdg_toplevel_close(void *data,
        struct xdg_toplevel *xdg_toplevel) {
    Locus *app = data;
    app->running = 0;
}

static void handle_geometry(void *data, struct wl_output *output,
        int32_t x, int32_t y, int32_t physical_width,
        int32_t physical_height, int32_t subpixel,
        const char *make, const char *model,
        int32_t transform) {
}

static void handle_mode(void *data, struct wl_output *output,
        uint32_t flags, int32_t width, int32_t height,
        int32_t refresh) {
    Locus *app = data;
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        app->screen_width = width;
        app->screen_height = height;
    }
}

static void handle_done(void *data, struct wl_output *output) {
}

static void handle_scale(void *data, struct wl_output *output, int32_t factor) {
}

static void touch_handle_down(void *data, struct wl_touch *wl_touch,
                              uint32_t serial, uint32_t time,
                              struct wl_surface *surface, int32_t id,
                              wl_fixed_t x, wl_fixed_t y) {
    Locus *app = data;
    double touch_x = wl_fixed_to_double(x);
    double touch_y = wl_fixed_to_double(y);

    if(app->touch_callback) {
        app->touch_callback(id, touch_x, touch_y, 0);
    }
}

static void touch_handle_up(void *data, struct wl_touch *wl_touch,
                            uint32_t serial, uint32_t time, int32_t id) {
    Locus *app = data;
    if(app->touch_callback) {
        app->touch_callback(id, 0, 0, 1);
    }
}

static void touch_handle_motion(void *data, struct wl_touch *wl_touch,
                                uint32_t time, int32_t id, wl_fixed_t x,
                                wl_fixed_t y) {
    Locus *app = data;
    double touch_x = wl_fixed_to_double(x);
    double touch_y = wl_fixed_to_double(y);
    if(app->touch_callback) {
        app->touch_callback(id, touch_x, touch_y, 2);
    }
}

static void touch_handle_frame(void *data, struct wl_touch *wl_touch) {
}

static const struct wl_touch_listener touch_listener = {
    .down = touch_handle_down,
    .up = touch_handle_up,
    .motion = touch_handle_motion,
    .frame = touch_handle_frame,
};

static void handle_seat_capabilities(void *data, struct wl_seat *seat, 
        uint32_t capabilities) {
    Locus *app = data;

    if (capabilities && WL_SEAT_CAPABILITY_TOUCH) {
        app->touch = wl_seat_get_touch(seat);
        wl_touch_add_listener(app->touch, &touch_listener, app);
    }
}

static void handle_seat_name(void *data, struct wl_seat *seat, 
        const char *name) {
}

static const struct wl_output_listener output_listener = {
    .geometry = handle_geometry,
    .mode = handle_mode,
    .done = handle_done,
    .scale = handle_scale,
};

static const struct wl_seat_listener seat_listener = {
    .capabilities = handle_seat_capabilities,
    .name = handle_seat_name,
};

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = handle_xdg_toplevel_configure,
    .close = handle_xdg_toplevel_close,
};

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = handle_configure,
};

static void handle_layer_surface_configure(
        void *data, struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial,
        uint32_t width, uint32_t height) {
    Locus *app = data;
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
    app->configured = 1;
    if (app->buffer) {
        app->draw_callback(app->cr, app->width, app->height);
        cairo_surface_flush(app->cairo_surface);
        wl_surface_attach(app->surface, app->buffer, 0, 0);
        wl_surface_damage(app->surface, 0, 0, app->width, app->height);
        wl_surface_commit(app->surface);
    }
}

static void
handle_layer_surface_closed(void *data,
        struct zwlr_layer_surface_v1 *layer_surface) {
    Locus *app = data;
    app->running = 0;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = handle_layer_surface_configure,
    .closed = handle_layer_surface_closed,
};

static void registry_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface,
        uint32_t version) {
    Locus *app = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor =
            wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        app->output=
            wl_registry_bind(registry, name, &wl_output_interface, 2);
        wl_output_add_listener(app->output, &output_listener, app);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        app->xdg_wm_base =
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(app->xdg_wm_base, &xdg_wm_base_listener, app);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        app->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        app->layer_shell =
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        app->seat =
            wl_registry_bind(registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(app->seat, &seat_listener, app);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
        uint32_t name) {
    // This space intentionally left blank
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static void create_buffer(Locus *app) {
    int stride = app->width * 4;
    int size = stride * app->height;

    char filename[] = "/tmp/wayland-shm-XXXXXX";
    char filename_back[] = "/tmp/wayland-shm-back-XXXXXX";
    
    int fd_front = mkstemp(filename);
    if (fd_front < 0) {
        fprintf(stderr, "Failed to create temp file for front buffer\n");
        exit(1);
    }
    unlink(filename);

    if (ftruncate(fd_front, size) == -1) {
        fprintf(stderr, "Failed to set size for front buffer temp file\n");
        close(fd_front);
        exit(1);
    }

    app->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_front, 0);
    if (app->shm_data == MAP_FAILED) {
        fprintf(stderr, "mmap failed for front buffer\n");
        close(fd_front);
        exit(1);
    }

    struct wl_shm_pool *pool_front = wl_shm_create_pool(app->shm, fd_front, size);
    if (!pool_front) {
        fprintf(stderr, "Failed to create Wayland shared memory pool for front buffer\n");
        munmap(app->shm_data, size);
        close(fd_front);
        exit(1);
    }

    app->buffer = wl_shm_pool_create_buffer(pool_front, 0, app->width, app->height,
            stride, WL_SHM_FORMAT_ARGB8888);
    if (!app->buffer) {
        fprintf(stderr, "Failed to create Wayland buffer for front buffer\n");
        wl_shm_pool_destroy(pool_front);
        munmap(app->shm_data, size);
        close(fd_front);
        exit(1);
    }

    wl_shm_pool_destroy(pool_front);
    close(fd_front);

    int fd_back = mkstemp(filename_back);
    if (fd_back < 0) {
        fprintf(stderr, "Failed to create temp file for back buffer\n");
        exit(1);
    }
    unlink(filename_back);

    if (ftruncate(fd_back, size) == -1) {
        fprintf(stderr, "Failed to set size for back buffer temp file\n");
        close(fd_back);
        exit(1);
    }

    app->shm_data_back = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_back, 0);
    if (app->shm_data_back == MAP_FAILED) {
        fprintf(stderr, "mmap failed for back buffer\n");
        close(fd_back);
        exit(1);
    }

    struct wl_shm_pool *pool_back = wl_shm_create_pool(app->shm, fd_back, size);
    if (!pool_back) {
        fprintf(stderr, "Failed to create Wayland shared memory pool for back buffer\n");
        munmap(app->shm_data_back, size);
        close(fd_back);
        exit(1);
    }

    app->buffer_back = wl_shm_pool_create_buffer(pool_back, 0, app->width, app->height,
            stride, WL_SHM_FORMAT_ARGB8888);
    if (!app->buffer_back) {
        fprintf(stderr, "Failed to create Wayland buffer for back buffer\n");
        wl_shm_pool_destroy(pool_back);
        munmap(app->shm_data_back, size);
        close(fd_back);
        exit(1);
    }

    wl_shm_pool_destroy(pool_back);
    close(fd_back);

    app->cairo_surface = cairo_image_surface_create_for_data(
            app->shm_data, CAIRO_FORMAT_ARGB32, app->width, app->height, stride);
    if (!app->cairo_surface) {
        fprintf(stderr, "Failed to create Cairo surface for front buffer\n");
        exit(1);
    }

    app->cairo_surface_back = cairo_image_surface_create_for_data(
            app->shm_data_back, CAIRO_FORMAT_ARGB32, app->width, app->height, stride);
    if (!app->cairo_surface_back) {
        fprintf(stderr, "Failed to create Cairo surface for back buffer\n");
        cairo_surface_destroy(app->cairo_surface);
        exit(1);
    }

    app->cr = cairo_create(app->cairo_surface);
    if (!app->cr) {
        fprintf(stderr, "Failed to create Cairo context for front buffer\n");
        cairo_surface_destroy(app->cairo_surface);
        cairo_surface_destroy(app->cairo_surface_back);
        exit(1);
    }

    app->cr_back = cairo_create(app->cairo_surface_back);
    if (!app->cr_back) {
        fprintf(stderr, "Failed to create Cairo context for back buffer\n");
        cairo_destroy(app->cr);
        cairo_surface_destroy(app->cairo_surface);
        cairo_surface_destroy(app->cairo_surface_back);
        exit(1);
    }
}

int locus_init(Locus *app, int width_percent, int height_percent) {
    memset(app, 0, sizeof(Locus));
    app->running = 1;

    app->display = wl_display_connect(NULL);
    if (!app->display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 0;
    }

    app->registry = wl_display_get_registry(app->display);
    wl_registry_add_listener(app->registry, &registry_listener, app);
    wl_display_roundtrip(app->display);

    if (!app->compositor || !app->xdg_wm_base || !app->shm ) {
        fprintf(stderr, "Failed to bind Wayland interfaces\n");
        return 0;
    }

    if (!app->output) {
        fprintf(stderr, "wl_output not found\n");
        return 0;
    }
    wl_display_roundtrip(app->display);

    if (app->screen_width == 0 || app->screen_height == 0) {
        fprintf(stderr, "Failed to retrieve dimensions\n");
        return 0;
    }
    
    app->width = (app->screen_width * width_percent) / 100;
    app->height = (app->screen_height * height_percent) / 100;

    create_buffer(app);
    return 1;
}

void locus_create_window(Locus *app, const char *title) {
    app->surface = wl_compositor_create_surface(app->compositor);
    app->xdg_surface =
        xdg_wm_base_get_xdg_surface(app->xdg_wm_base, app->surface);
    xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);
    app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);
    xdg_toplevel_add_listener(app->xdg_toplevel, &xdg_toplevel_listener, app);

    xdg_toplevel_set_title(app->xdg_toplevel, title);
    wl_surface_commit(app->surface);
}

void locus_create_layer_surface(Locus *app, uint32_t layer, uint32_t anchor) {
    app->surface = wl_compositor_create_surface(app->compositor);
    app->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            app->layer_shell, app->surface, NULL, layer, "wayland-app-layer");
    zwlr_layer_surface_v1_set_size(app->layer_surface, app->width, app->height);
    zwlr_layer_surface_v1_set_anchor(app->layer_surface, anchor);
    zwlr_layer_surface_v1_add_listener(app->layer_surface,
            &layer_surface_listener, app);
    wl_surface_commit(app->surface);
}

void locus_set_draw_callback(Locus *app,
        void (*draw_callback)(cairo_t *cr, int width,
            int height)) {
    app->draw_callback = draw_callback;
}

void locus_set_touch_callback(Locus *app,
        void (*touch_callback)(int32_t id, double x,
            double y, int32_t state)) {
    app->touch_callback = touch_callback;
}

void locus_set_partial_draw_callback(Locus *app,
        void (*partial_draw_callback)(cairo_t *cr, int x, int y, 
            int width, int height)) {
    app->partial_draw_callback = partial_draw_callback;
}

void locus_req_partial_redraw(Locus *app, int x, int y, int width, int height) {
    app->redraw_partial = 1;
    app->redraw_x = x;
    app->redraw_y = y;
    app->redraw_width = width;
    app->redraw_height = height;
}

void locus_run(Locus *app) {
    while (!app->configured) {
        wl_display_dispatch(app->display);
    }

    app->draw_callback(app->cr, app->width, app->height);
    app->redraw = 1;
    cairo_surface_flush(app->cairo_surface);
    wl_surface_attach(app->surface, app->buffer, 0, 0);
    wl_surface_damage(app->surface, 0, 0, app->width, app->height);
    wl_surface_commit(app->surface);

    while (app->running) {
        while (wl_display_prepare_read(app->display) != 0) {
            wl_display_dispatch_pending(app->display);
        }
        wl_display_flush(app->display);
        if (app->redraw) {
            app->draw_callback(app->cr_back, app->width, app->height);
            cairo_surface_flush(app->cairo_surface_back);
            wl_surface_attach(app->surface, app->buffer_back, 0, 0);
            wl_surface_damage(app->surface, 0, 0, app->width, app->height);
            wl_surface_commit(app->surface);
            app->redraw = 0;
        } else if (app->redraw_partial) {
            app->partial_draw_callback(app->cr_back, app->redraw_x, app->redraw_y, app->redraw_width, app->redraw_height);
            cairo_surface_flush(app->cairo_surface_back);
            wl_surface_attach(app->surface, app->buffer_back, 0, 0);
            wl_surface_damage(app->surface, app->redraw_x, app->redraw_y, app->redraw_width, app->redraw_height);
            wl_surface_commit(app->surface);
            app->redraw_partial = 0;
        }
        wl_display_read_events(app->display);
        wl_display_dispatch_pending(app->display);
    }
}

void locus_cleanup(Locus *app) {
    if (app->cr) {
        cairo_destroy(app->cr);
    }
    if (app->cr_back) {
        cairo_destroy(app->cr_back);
    }
    if (app->cairo_surface) {
        cairo_surface_destroy(app->cairo_surface);
        if (app->shm_data) {
            munmap(app->shm_data, app->width * app->height * 4);
        }
    }
    if (app->cairo_surface_back) {
        cairo_surface_destroy(app->cairo_surface_back);
        if (app->shm_data_back) {
            munmap(app->shm_data_back, app->width * app->height * 4);
        }
    }
    if (app->buffer) {
        wl_buffer_destroy(app->buffer);
    }
    if (app->buffer_back) {
        wl_buffer_destroy(app->buffer_back);
    }
    if (app->xdg_toplevel) {
        xdg_toplevel_destroy(app->xdg_toplevel);
    }
    if (app->xdg_surface) {
        xdg_surface_destroy(app->xdg_surface);
    }
    if (app->layer_surface) {
        zwlr_layer_surface_v1_destroy(app->layer_surface);
    }
    if (app->output) {
        wl_output_destroy(app->output);
    }
    if (app->xdg_toplevel) {
        xdg_toplevel_destroy(app->xdg_toplevel);
    }
    if (app->xdg_surface) {
        xdg_surface_destroy(app->xdg_surface);
    }
    if (app->layer_surface) {
        zwlr_layer_surface_v1_destroy(app->layer_surface);
    }
    if (app->output) {
        wl_output_destroy(app->output);
    }
    if (app->surface) {
        wl_surface_destroy(app->surface);
    }
    if (app->xdg_wm_base) {
        xdg_wm_base_destroy(app->xdg_wm_base);
    }
    if (app->compositor) {
        wl_compositor_destroy(app->compositor);
    }
    if (app->registry) {
        wl_registry_destroy(app->registry);
    }
    if (app->seat) {
        wl_seat_destroy(app->seat);
    }
    if (app->touch) {
        wl_touch_destroy(app->touch);
    }
    
    if (app->display) {
        wl_display_disconnect(app->display);
    }
}
