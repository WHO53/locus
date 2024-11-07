#include <librsvg/rsvg.h>
#include <cairo.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "locus-ui.h"

void locus_gen_png(const char* icon_name) {
    RsvgHandle *handle;
    GError *error = NULL;
    cairo_surface_t *surface;
    cairo_t *cr;
    char filename[512];
    char output_filename[512];

    const char* dirs[] = {
        "/home/droidian/temp/Fluent-grey-dark/scalable/apps/",
        "/home/droidian/temp/Fluent-grey-dark/symbolic/apps/"
    };

    int found = 0;
    for (int i = 0; i < 2; i++) {
        snprintf(filename, sizeof(filename), "%s%s.svg", dirs[i], icon_name);
        if (access(filename, F_OK) != -1) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("SVG file for icon %s not found in the specified directories.\n", icon_name);
        return;
    }

    handle = rsvg_handle_new_from_file(filename, &error);
    if (error) {
        g_printerr("Error loading SVG: %s\n", error->message);
        g_error_free(error);
        return;
    }

    gdouble width, height;
    gboolean success = rsvg_handle_get_intrinsic_size_in_pixels(handle, &width, &height);
    if (!success) {
        g_printerr("Error getting SVG dimensions\n");
        g_object_unref(handle);
        return;
    }

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
    cr = cairo_create(surface);

    RsvgRectangle viewport = {0, 0, width, height};

    success = rsvg_handle_render_document(handle, cr, &viewport, &error);
    if (!success) {
        if (error) {
            g_printerr("Error rendering SVG to Cairo context: %s\n", error->message);
            g_error_free(error);
        }
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_object_unref(handle);
        return;
    }

    snprintf(output_filename, sizeof(output_filename), "/home/droidian/.local/share/locus/%s.png", icon_name);

    cairo_surface_write_to_png(surface, output_filename);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(handle);
}
