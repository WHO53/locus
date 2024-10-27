#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <librsvg/rsvg.h>
#include "locus-ui.h"

void locus_rectangle(cairo_t *cr, double center_x, double center_y, double width, double height, double radius, unsigned int corner_flags) {
    double x = center_x - width / 2.0;
    double y = center_y - height / 2.0;
    cairo_new_path(cr);
    if (corner_flags & TOP_LEFT) {
        cairo_move_to(cr, x + radius, y);
    } else {
        cairo_move_to(cr, x, y);
    }
    if (corner_flags & TOP_RIGHT) {
        cairo_line_to(cr, x + width - radius, y);
        cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI / 2, 0);
    } else {
        cairo_line_to(cr, x + width, y);
    }
    if (corner_flags & BOTTOM_RIGHT) {
        cairo_line_to(cr, x + width, y + height - radius);
        cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI / 2);
    } else {
        cairo_line_to(cr, x + width, y + height);
    }
    if (corner_flags & BOTTOM_LEFT) {
        cairo_line_to(cr, x + radius, y + height);
        cairo_arc(cr, x + radius, y + height - radius, radius, M_PI / 2, M_PI);
    } else {
        cairo_line_to(cr, x, y + height);
    }
    if (corner_flags & TOP_LEFT) {
        cairo_line_to(cr, x, y + radius);
        cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);
    } else {
        cairo_line_to(cr, x, y);
    }
    cairo_close_path(cr);
    cairo_fill(cr);
}

int locus_file_exists(const char *path) {
    FILE *file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

char *locus_find_icon(const char *icon_name) {
    static char path[1024];
    const char *icon_dirs[] = {
        "/home/droidian/temp/Fluent-grey-dark/scalable/apps/",
        "/home/droidian/temp/Fluent-grey-dark/symbolic/status/",
        "/usr/share/icons/hicolor/scalable/apps/",
        "/usr/share/icons/hicolor/128x128/apps/",
        "/usr/share/icons/Adwaita/symbolic/status/",
        "/usr/share/pixmaps/",
        NULL
    };
    
    for (int i = 0; icon_dirs[i] != NULL; ++i) {
        if (strcmp(icon_dirs[i], "/usr/share/pixmaps/") == 0) {
            snprintf(path, sizeof(path), "%s%s.png", icon_dirs[i], icon_name);
            if (locus_file_exists(path)) return path;
        } else {
            snprintf(path, sizeof(path), "%s%s.svg", icon_dirs[i], icon_name);
            if (locus_file_exists(path)) return path;
            snprintf(path, sizeof(path), "%s%s-symbolic.svg", icon_dirs[i], icon_name);
            if (locus_file_exists(path)) return path;
        }
    }
    return NULL;
}

void locus_icon(cairo_t *cr, double center_x, double center_y, const char *icon_name, double width, double height) { 
    double x = center_x - width / 2.0;
    double y = center_y - height / 2.0;
    char *icon_path = locus_find_icon(icon_name);
    cairo_surface_t *icon_surface = NULL;

    if (icon_path != NULL) {
        if (strstr(icon_path, ".svg")) {
            RsvgHandle *svg = rsvg_handle_new_from_file(icon_path, NULL);
            RsvgRectangle viewport = { 0, 0, width, height };
            if (svg) {
                cairo_save(cr);
                cairo_translate(cr, x, y);
                rsvg_handle_render_document(svg, cr, &viewport, NULL);
                cairo_restore(cr);
                g_object_unref(svg);
            }
        } else {
            icon_surface = cairo_image_surface_create_from_png(icon_path);
            if (cairo_surface_status(icon_surface) == CAIRO_STATUS_SUCCESS) {
                double icon_width = cairo_image_surface_get_width(icon_surface);
                double icon_height = cairo_image_surface_get_height(icon_surface);
                cairo_save(cr);
                cairo_translate(cr, x, y);
                cairo_scale(cr, (double)width / icon_width, (double)height / icon_height);
                cairo_set_source_surface(cr, icon_surface, 0, 0);
                cairo_paint(cr);
                cairo_restore(cr);
                cairo_surface_destroy(icon_surface);
            }
        }
    }

    if (!icon_surface && !icon_path) {
        cairo_set_source_rgb(cr, 0.2, 0.6, 0.8);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
    }
}

void locus_text(cairo_t *cr, const char *text, double center_x, double center_y, double width, double height, FontWeight font_weight){
    cairo_font_weight_t weight;
    weight = (font_weight == BOLD) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;
    cairo_select_font_face(cr, "Monofur Nerd Font", 
            CAIRO_FONT_SLANT_NORMAL, weight);
    cairo_set_font_size(cr, height);
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);
    
    cairo_move_to(cr,
            center_x - extents.width / 2,
            center_y + extents.height / 2); 
    cairo_show_text(cr, text);
}
