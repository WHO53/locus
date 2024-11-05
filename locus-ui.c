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
                    float red, float green, float blue, float alpha) {
    nvgBeginPath(ui->vg);
    nvgRect(ui->vg, x, y, width, height);
    nvgFillColor(ui->vg, nvgRGBA((int)(red * 255), (int)(green * 255), (int)(blue * 255), (int)(alpha * 255)));
    nvgFill(ui->vg);
}

void locus_cleanup_ui(LocusUI* ui) {
    nvgDeleteGLES2(ui->vg); 
}
