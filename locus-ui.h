#ifndef LOCUS_UI_H
#define LOCUS_UI_H

#include <cairo.h>

enum RoundedCorner {
    TOP_LEFT = 1 << 0,
    TOP_RIGHT = 1 << 1,
    BOTTOM_RIGHT = 1 << 2,
    BOTTOM_LEFT = 1 << 3
};

typedef enum { BOLD, NORMAL } FontWeight;

void locus_rectangle(cairo_t *cr, double x, double y, double width, double height, double radius, unsigned int corner_flags);

void locus_icon(cairo_t *cr, double x, double y, const char *icon_name, double width, double height);

void locus_text(cairo_t *cr, const char *text, double x, double y, double width, double height, FontWeight font_weight);

#endif // LOCUS_UI_H
