#include "locus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

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
  Locus *app = data;
  if (width > 0 && height > 0) {
    app->width = width;
    app->height = height;
    // Recreate buffer with new size
    if (app->buffer) {
      wl_buffer_destroy(app->buffer);
    }
    if (app->cairo_surface) {
      cairo_surface_destroy(app->cairo_surface);
    }
    if (app->cr) {
      cairo_destroy(app->cr);
    }
    create_buffer(app);
  }
}

static void handle_xdg_toplevel_close(void *data,
                                      struct xdg_toplevel *xdg_toplevel) {
  Locus *app = data;
  app->running = 0;
}

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
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    app->xdg_wm_base =
        wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(app->xdg_wm_base, &xdg_wm_base_listener, app);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    app->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
    app->layer_shell =
        wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
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
  int fd = mkstemp(filename);
  if (fd < 0) {
    fprintf(stderr, "Failed to create temp file\n");
    exit(1);
  }
  unlink(filename);
  ftruncate(fd, size);

  app->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (app->shm_data == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    exit(1);
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(app->shm, fd, size);
  app->buffer = wl_shm_pool_create_buffer(pool, 0, app->width, app->height,
                                          stride, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  app->cairo_surface = cairo_image_surface_create_for_data(
      app->shm_data, CAIRO_FORMAT_ARGB32, app->width, app->height, stride);
  app->cr = cairo_create(app->cairo_surface);
}

int locus_init(Locus *app, int width, int height) {
  memset(app, 0, sizeof(Locus));
  app->width = width;
  app->height = height;
  app->running = 1;

  app->display = wl_display_connect(NULL);
  if (!app->display) {
    fprintf(stderr, "Failed to connect to Wayland display\n");
    return 0;
  }

  app->registry = wl_display_get_registry(app->display);
  wl_registry_add_listener(app->registry, &registry_listener, app);
  wl_display_roundtrip(app->display);

  if (!app->compositor || !app->xdg_wm_base || !app->shm) {
    fprintf(stderr, "Failed to bind Wayland interfaces\n");
    return 0;
  }

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

void locus_run(Locus *app) {
  while (!app->configured) {
    wl_display_dispatch(app->display);
  }

  app->draw_callback(app->cr, app->width, app->height);
  cairo_surface_flush(app->cairo_surface);
  wl_surface_attach(app->surface, app->buffer, 0, 0);
  wl_surface_damage(app->surface, 0, 0, app->width, app->height);
  wl_surface_commit(app->surface);

  while (app->running && wl_display_dispatch(app->display) != -1) {
    // This space intentionally left blank
  }
}

void locus_cleanup(Locus *app) {
  cairo_destroy(app->cr);
  cairo_surface_destroy(app->cairo_surface);
  wl_buffer_destroy(app->buffer);
  if (app->xdg_toplevel)
    xdg_toplevel_destroy(app->xdg_toplevel);
  if (app->xdg_surface)
    xdg_surface_destroy(app->xdg_surface);
  if (app->layer_surface)
    zwlr_layer_surface_v1_destroy(app->layer_surface);
  wl_surface_destroy(app->surface);
  xdg_wm_base_destroy(app->xdg_wm_base);
  wl_compositor_destroy(app->compositor);
  wl_registry_destroy(app->registry);
  wl_display_disconnect(app->display);
}
