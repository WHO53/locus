#ifndef LOCUS_UI_H
#define LOCUS_UI_H

#include <nanovg.h>

typedef struct {
    NVGcontext* vg;
} LocusUI;

void locus_setup_ui(LocusUI* ui);  

void locus_rectangle(LocusUI* ui, float x, float y, float width, float height, 
                     float red, float green, float blue, float alpha, float cornerRadius);

void locus_text(LocusUI* ui, const char* text, float x, float y, 
                float fontSize, float red, float green, float blue, float alpha);

void locus_cleanup_ui(LocusUI* ui);  

#endif 
