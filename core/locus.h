#ifndef LOCUS_H
#define LOCUS_H

#include <wayland-client.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "proto/wlr-layer-shell-unstable-v1-client-protocol.h"
#include "proto/xdg-shell-client-protocol.h"

typedef struct Locus Locus;

struct Locus {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_output *output;
    struct wl_seat *seat;
    struct wl_touch *touch;
    struct wl_egl_window *egl_window;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLConfig egl_config;
    EGLSurface egl_surface;
    struct xdg_wm_base *xdg_wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zwlr_layer_surface_v1 *layer_surface;
    int width, height;
    int screen_width, screen_height;
    int configured;
    int running;
    int redraw;
    int active_touches;
    void (*draw_callback)(void *data);
    void (*touch_callback)(int32_t id, double x, double y, int32_t state);
};

int locus_init(Locus *app, int width, int height);
void locus_create_window(Locus *app, const char *title);
void locus_create_layer_surface(Locus *app, const char *title, uint32_t layer, uint32_t anchor, int exclusive);
void locus_set_draw_callback(Locus *app, void (*draw_callback)(void *data));
void locus_set_touch_callback(Locus *app, void (*touch_callback)(int32_t id, double x, double y, int32_t state));
void locus_run(Locus *app);
void locus_cleanup(Locus *app);

#endif 
