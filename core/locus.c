#include "locus.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>

static void handle_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = handle_ping,
};

static void handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    Locus *app = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    app->configured = 1;
    if (app->egl_surface) {
        app->redraw = 1;
    }
}

static void handle_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                          int32_t width, int32_t height, struct wl_array *states) {
    Locus *app = data;
    if (width > 0 && height > 0) {
        if (app->egl_window) {
            app->redraw = 1;
        }
    }
}

static void handle_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    Locus *app = data;
    app->xdg_toplevel = NULL;
    wl_egl_window_destroy(app->egl_window);
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

    app->active_touches++;

    if(app->active_touches == 1 && app->touch_callback) {
        app->touch_callback(id, touch_x, touch_y, 0);
    }
}

static void touch_handle_up(void *data, struct wl_touch *wl_touch,
                            uint32_t serial, uint32_t time, int32_t id) {
    Locus *app = data;
    
    if (app->active_touches > 0) {
        app->active_touches--;
    }

    if(app->active_touches == 0 && app->touch_callback) {
        app->touch_callback(id, 0, 0, 1);
    }
}

static void touch_handle_motion(void *data, struct wl_touch *wl_touch,
                                uint32_t time, int32_t id, wl_fixed_t x,
                                wl_fixed_t y) {
    Locus *app = data;
    double touch_x = wl_fixed_to_double(x);
    double touch_y = wl_fixed_to_double(y);
    if(app->active_touches == 1 && app->touch_callback) {
        app->touch_callback(id, touch_x, touch_y, 2);
    }
}

static void touch_handle_frame(void *data, struct wl_touch *wl_touch) {
}

static void touch_handle_cancel(void *data, struct wl_touch *wl_touch) {
    Locus *app = data;
    app->active_touches = 0;
    if (app->touch_callback) {
        app->touch_callback(-1, 0, 0, 3);
    }
}

static const struct wl_touch_listener touch_listener = {
    .down = touch_handle_down,
    .up = touch_handle_up,
    .motion = touch_handle_motion,
    .frame = touch_handle_frame,
    .cancel = touch_handle_cancel,
};

static void handle_seat_capabilities(void *data, struct wl_seat *seat, 
                                     uint32_t capabilities) {
    Locus *app = data;

    if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
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

static void handle_layer_surface_configure(void *data, 
                                           struct zwlr_layer_surface_v1 *layer_surface,
                                           uint32_t serial, uint32_t width, uint32_t height) {
    Locus *app = data;
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

    if (width > 0 && height > 0) {
        if (app->egl_window) {
            app->redraw = 1;
        }
    }
    app->configured = 1;
    wl_display_roundtrip(app->display);
    wl_surface_commit(app->surface);
}

static void handle_layer_surface_closed(void *data,
                                        struct zwlr_layer_surface_v1 *layer_surface) {
    Locus *app = data;
    app->layer_surface = NULL;
    wl_egl_window_destroy(app->egl_window);
    app->running = 0;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = handle_layer_surface_configure,
    .closed = handle_layer_surface_closed,
};

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = handle_xdg_toplevel_configure,
    .close = handle_xdg_toplevel_close,
};

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = handle_configure,
};

static void init_egl(Locus *app) {
    EGLint major, minor, count, n;
    EGLConfig *configs;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = 
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

    if (get_platform_display) {
        app->egl_display = get_platform_display(EGL_PLATFORM_WAYLAND_EXT, app->display, NULL);
    } else {
        app->egl_display = eglGetDisplay((EGLNativeDisplayType)app->display);
    }

    eglInitialize(app->egl_display, &major, &minor);
    eglGetConfigs(app->egl_display, NULL, 0, &count);
    configs = calloc(count, sizeof *configs);

    eglChooseConfig(app->egl_display, config_attribs, configs, count, &n);
    app->egl_config = configs[0];

    app->egl_context = eglCreateContext(app->egl_display, app->egl_config, 
                                        EGL_NO_CONTEXT, context_attribs);
    free(configs);
}

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface, uint32_t version) {
    Locus *app = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        app->output= wl_registry_bind(registry, name, &wl_output_interface, 2);
        wl_output_add_listener(app->output, &output_listener, app);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        app->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(app->xdg_wm_base, &xdg_wm_base_listener, app);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        app->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        app->seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(app->seat, &seat_listener, app);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {

}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

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

    if (!app->compositor || !app->xdg_wm_base) {
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

    init_egl(app);
    return 1;
}

void locus_create_window(Locus *app, const char *title) {
    app->surface = wl_compositor_create_surface(app->compositor);
    app->xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base, app->surface);
    xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);
    app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);
    xdg_toplevel_add_listener(app->xdg_toplevel, &xdg_toplevel_listener, app);

    xdg_toplevel_set_title(app->xdg_toplevel, title);

    app->egl_window = wl_egl_window_create(app->surface, app->width, app->height);
    app->egl_surface = eglCreateWindowSurface(app->egl_display, app->egl_config, 
                                              (EGLNativeWindowType)app->egl_window, NULL);
    eglMakeCurrent(app->egl_display, app->egl_surface, app->egl_surface, app->egl_context);

    wl_surface_commit(app->surface);
}

void locus_create_layer_surface(Locus *app, const char *title, uint32_t layer, 
                                uint32_t anchor, int exclusive) {
    app->surface = wl_compositor_create_surface(app->compositor);
    app->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        app->layer_shell, app->surface, NULL, layer, title);

    zwlr_layer_surface_v1_set_size(app->layer_surface, app->width, app->height);
    zwlr_layer_surface_v1_set_anchor(app->layer_surface, anchor);
    if (exclusive) {
        zwlr_layer_surface_v1_set_exclusive_zone(app->layer_surface, app->height);
    }

    zwlr_layer_surface_v1_add_listener(app->layer_surface, &layer_surface_listener, app);

    app->egl_window = wl_egl_window_create(app->surface, app->width, app->height);
    app->egl_surface = eglCreateWindowSurface(app->egl_display, app->egl_config, 
                                              (EGLNativeWindowType)app->egl_window, NULL);
    eglMakeCurrent(app->egl_display, app->egl_surface, app->egl_surface, app->egl_context);

    wl_surface_commit(app->surface);
}

void locus_set_draw_callback(Locus *app, void (*draw_callback)(void *data)) {
    app->draw_callback = draw_callback;
}

void locus_set_touch_callback(Locus *app,
                              void (*touch_callback)(int32_t id, double x,
                                                     double y, int32_t state)) {
    app->touch_callback = touch_callback;
}

void locus_run(Locus *app) {

    while (!app->configured) {
        wl_display_dispatch(app->display);
    }

    app->redraw = 1; 
    while (app->running) {

        if (wl_display_prepare_read(app->display) != 0) {
            wl_display_dispatch_pending(app->display);
        } else {
            wl_display_flush(app->display);
            struct timespec ts = {0, 16667000};
            nanosleep(&ts, NULL);
        }

        if (app->redraw) {
            eglMakeCurrent(app->egl_display, app->egl_surface, app->egl_surface, app->egl_context);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            if (app->draw_callback) {
                app->draw_callback(app);
            } else {
                fprintf(stderr, "Draw callback not set\n");
            }

            wl_surface_damage(app->surface, 0, 0, app->width, app->height);

            if (eglSwapBuffers(app->egl_display, app->egl_surface) == EGL_FALSE) {
                fprintf(stderr, "Failed to swap buffers\n");
            }

            wl_surface_commit(app->surface);

            app->redraw = 0; 
        }
        wl_display_read_events(app->display);
        wl_display_dispatch_pending(app->display);
    }
}


void locus_cleanup(Locus *app) {
    if (app->egl_surface) {
        eglDestroySurface(app->egl_display, app->egl_surface);
        app->egl_surface = NULL;
    }
    if (app->egl_window) {
        wl_egl_window_destroy(app->egl_window);
        app->egl_window = NULL;
    }
    if (app->egl_context) {
        eglDestroyContext(app->egl_display, app->egl_context);
        app->egl_context = NULL;
    }
    if (app->egl_display) {
        eglTerminate(app->egl_display);
        app->egl_display = NULL;
    }
    if (app->xdg_toplevel) {
        xdg_toplevel_destroy(app->xdg_toplevel);
        app->xdg_toplevel = NULL;
    }
    if (app->xdg_surface) {
        xdg_surface_destroy(app->xdg_surface);
        app->xdg_surface = NULL;
    }
    if (app->layer_surface) {
        zwlr_layer_surface_v1_destroy(app->layer_surface);
        app->layer_surface = NULL;
    }
    if (app->surface) {
        wl_surface_destroy(app->surface);
        app->surface = NULL;
    }
    if (app->xdg_wm_base) {
        xdg_wm_base_destroy(app->xdg_wm_base);
        app->xdg_wm_base = NULL;
    }
    if (app->compositor) {
        wl_compositor_destroy(app->compositor);
        app->compositor = NULL;
    }
    if (app->registry) {
        wl_registry_destroy(app->registry);
        app->registry = NULL;
    }
    if (app->display) {
        wl_display_disconnect(app->display);
        app->display = NULL;
    }
}
