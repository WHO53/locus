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

void locus_image(LocusUI* ui, const char* imagePath, float x, float y, float width, float height) {

    int image = nvgCreateImage(ui->vg, imagePath, 0); 
    if (image == 0) {
        fprintf(stderr, "Failed to load image: %s\n", imagePath);
        return;
    }

    int imgWidth, imgHeight;
    nvgImageSize(ui->vg, image, &imgWidth, &imgHeight);

    if (imgWidth == 0 || imgHeight == 0) {
        fprintf(stderr, "Image dimensions are invalid.\n");
        nvgDeleteImage(ui->vg, image);  
        return;
    }

    float iw, ih, ix = 0.0f, iy = 0.0f;
    if (imgWidth < imgHeight) {
        iw = width;
        ih = iw * imgHeight / imgWidth;
        iy = -(ih - height) * 0.5f;
    } else {
        ih = height;
        iw = ih * imgWidth / imgHeight;
        ix = -(iw - width) * 0.5f;
    }

    NVGpaint imgPaint = nvgImagePattern(ui->vg, x + ix, y + iy, iw, ih, 0.0f, image, 1.0f);

    nvgBeginPath(ui->vg);
    nvgRect(ui->vg, x, y, width, height);
    nvgFillPaint(ui->vg, imgPaint);  
    nvgFill(ui->vg);  
}

void locus_cleanup_ui(LocusUI* ui) {
    nvgDeleteGLES2(ui->vg); 
}
