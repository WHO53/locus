#ifndef LOCUS_UI_H
#define LOCUS_UI_H

#include <GLES2/gl2.h>  

void locus_setup_gles();

void locus_cleanup_gles();

void locus_rectangle(float x, float y, float width, float height, 
                    float red, float green, float blue, float alpha);

#endif 
