#ifndef LOCUS_H
#define LOCUS_H

#include "proto/wlr-layer-shell-unstable-v1-client-protocol.h"
#include "proto/xdg-shell-client-protocol.h"
#include <cairo/cairo.h>
#include <wayland-client.h>

typedef struct {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_output *output;
    struct xdg_wm_base *xdg_wm_base;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_shm *shm;
    struct wl_buffer *buffer;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zwlr_layer_surface_v1 *layer_surface;
    cairo_surface_t *cairo_surface;
    cairo_t *cr;
    int width, height;
    int screen_width, screen_height;
    void *shm_data;
    int configured;
    int running;
    void (*draw_callback)(cairo_t *cr, int width, int height);
} Locus;

// Initialize the Wayland application
int locus_init(Locus *app, int width, int height);

// Create a regular window
void locus_create_window(Locus *app, const char *title);

// Create a layer shell surface
void locus_create_layer_surface(Locus *app, uint32_t layer, uint32_t anchor);

// Set the draw callback function
void locus_set_draw_callback(Locus *app,
        void (*draw_callback)(cairo_t *cr, int width,
            int height));

// Run the main loop
void locus_run(Locus *app);

// Clean up resources
void locus_cleanup(Locus *app);

#endif // LOCUS_H
