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
    struct wl_seat *seat;
    struct wl_touch *touch;
    struct xdg_wm_base *xdg_wm_base;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_shm *shm;
    struct wl_buffer *buffer;
    struct wl_buffer *buffer_back;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zwlr_layer_surface_v1 *layer_surface;
    cairo_surface_t *cairo_surface;
    cairo_surface_t *cairo_surface_back;
    cairo_t *cr;
    cairo_t *cr_back;
    int width, height;
    int screen_width, screen_height;
    void *shm_data;
    void *shm_data_back;
    int configured;
    int running;
    int redraw;
    int redraw_partial;
    int redraw_x, redraw_y, redraw_width, redraw_height;
    void (*draw_callback)(cairo_t *cr, int width, int height);
    void (*touch_callback)(int32_t id, double x, double y, int32_t state);
    void (*partial_draw_callback)(cairo_t *cr, int x, int y, int width, int height);
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

void locus_set_touch_callback(Locus *app,
        void (*touch_callback)(int32_t id, double x,
            double y, int32_t state));

void locus_set_partial_draw_callback(Locus *app,
        void (*partial_draw_callback)(cairo_t *cr, int x, int y, 
            int width, int height));

void locus_req_partial_redraw(Locus *app, int x, int y, int width, int height);

// Run the main loop
void locus_run(Locus *app);

// Clean up resources
void locus_cleanup(Locus *app);

#endif // LOCUS_H
