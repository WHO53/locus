#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "locus-ui.h"

void locus_setup_ui(LocusUI* ui) {
    ui->vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!ui->vg) {
        fprintf(stderr, "Could not init NanoVG.\n");
        exit(EXIT_FAILURE);
    }
}

void locus_rectangle(LocusUI* ui, float x, float y, float width, float height, 
                     float red, float green, float blue, float alpha, float cornerRadius) {
    nvgBeginPath(ui->vg);
    nvgRoundedRect(ui->vg, x, y, width, height, cornerRadius);
    nvgFillColor(ui->vg, nvgRGBA((int)(red * 255), (int)(green * 255), (int)(blue * 255), (int)(alpha * 255)));
    nvgFill(ui->vg);
}

void locus_text(LocusUI* ui, const char* text, float x, float y, 
                float fontSize, float red, float green, float blue, float alpha) {
    nvgBeginPath(ui->vg);  
    nvgCreateFont(ui->vg, "font", "/home/droidian/.local/share/fonts/MonofurNerdFont-Regular.ttf");
    nvgFontFace(ui->vg, "font");  
    nvgFontSize(ui->vg, fontSize);
    nvgTextAlign(ui->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);  
    nvgFillColor(ui->vg, nvgRGBA((int)(red * 255), (int)(green * 255), (int)(blue * 255), (int)(alpha * 255)));
    nvgText(ui->vg, x, y, text, NULL);
}
void locus_cleanup_ui(LocusUI* ui) {
    nvgDeleteGLES2(ui->vg); 
}
