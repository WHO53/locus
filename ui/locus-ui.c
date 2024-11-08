#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "locus-ui.h"
#include <unistd.h>
#include <nanosvg.h>
#include <nanosvgrast.h>

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
    nvgFillColor(ui->vg, nvgRGBA(red, green, blue, (int)(alpha * 255)));
    nvgFill(ui->vg);
}

void locus_text(LocusUI* ui, const char* text, float x, float y, 
                float fontSize, float red, float green, float blue, float alpha) {
    nvgBeginPath(ui->vg);  
    nvgCreateFont(ui->vg, "font", "/home/droidian/.local/share/fonts/MonofurNerdFont-Regular.ttf");
    nvgFontFace(ui->vg, "font");  
    nvgFontSize(ui->vg, fontSize);
    nvgTextAlign(ui->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);  
    nvgFillColor(ui->vg, nvgRGBA(red, green, blue, (int)(alpha * 255)));
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

int file_exists(const char* filename) {
    return access(filename, F_OK) != -1;
}

void locus_icon(LocusUI* ui, const char* icon_name, float x, float y, float size) {
    char icon_path_svg[512];
    char icon_path_png[512];

    const char* svg_dirs[] = {
        "/home/droidian/temp/Fluent-grey-dark/scalable/apps/",
        "/home/droidian/temp/McMojave-circle-blue-light/apps/scalable/",
        "/home/droidian/temp/McMojave-circle-blue-dark/apps/symbolic/",
        "/home/droidian/temp/McMojave-circle-blue-dark/actions/symbolic/",
        "/home/droidian/temp/Fluent-grey-dark/symbolic/apps/",
        "/home/droidian/temp/Fluent-grey-dark/symbolic/actions/",
        "/usr/share/icons/hicolor/scalable/apps/",
    };
    
    int found_svg = 0;
    for (int i = 0; i < 7; i++) {
        snprintf(icon_path_svg, sizeof(icon_path_svg), "%s%s.svg", svg_dirs[i], icon_name);
        if (file_exists(icon_path_svg)) {
            found_svg = 1;
            break;
        }
    }
    
    if (!found_svg) {
        snprintf(icon_path_png, sizeof(icon_path_png), "/usr/share/pixmaps/%s.png", icon_name);
        if (!file_exists(icon_path_png)) {
            fprintf(stderr, "Error: Icon '%s' not found (neither SVG nor PNG)\n", icon_name);
            return;
        }

        locus_image(ui, icon_path_png, x, y, size, size);
        return;
    }

    
    NSVGimage* svg = nsvgParseFromFile(icon_path_svg, "px", 96.0f);
    if (svg == NULL) {
        fprintf(stderr, "Error: Failed to load SVG icon '%s'\n", icon_name);
        return;
    }

    struct NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (rast == NULL) {
        nsvgDelete(svg);
        fprintf(stderr, "Error: Could not create SVG rasterizer\n");
        return;
    }

    int imgWidth = size;
    int imgHeight = size;
    float scale = svg->width > svg->height ? size / svg->width : size / svg->height;
    uint8_t* data = malloc(imgWidth * imgHeight * 4);
    if (data == NULL) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(svg);
        fprintf(stderr, "Error: Could not allocate memory for SVG rasterization\n");
        return;
    }

    nsvgRasterize(rast, svg, 0, 0, scale, data, imgWidth, imgHeight, imgWidth * 4);
    
    int imgHandle = nvgCreateImageRGBA(ui->vg, imgWidth, imgHeight, 0, data);
    free(data);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg);

    if (imgHandle == 0) {
        fprintf(stderr, "Error: Failed to create NanoVG image from SVG '%s'\n", icon_name);
        return;
    }

    NVGpaint imgPaint = nvgImagePattern(ui->vg, x, y, size, size, 0, imgHandle, 1.0f);
    nvgBeginPath(ui->vg);
    nvgRect(ui->vg, x, y, size, size);  
    nvgFillPaint(ui->vg, imgPaint);     
    nvgFill(ui->vg);                    
}

void locus_cleanup_ui(LocusUI* ui) {
    if (ui->vg) {
        nvgDeleteGLES2(ui->vg); 
        ui->vg = NULL;
    }
}
