#ifndef UI_H
#define UI_H

#include <cairo/cairo.h>

// cant think of anything more for now
void locus_panel(cairo_t *cr, int width, int height); // does panel even need partial redraw? then wtf doesn't?
void locus_button(cairo_t *cr, int x, int y, int width, int height);
void locus_list(cairo_t *cr, int x, int y, int width, int height);
void locus_popup(cairo_t *cr, int x, int y, int width, int height);

#endif // UI_H
