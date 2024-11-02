#include "locus-ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static GLuint shader_program;
static GLint position_attr, color_uniform;

// Vertex shader source code
const char* vertex_shader_source =
    "attribute vec4 position;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "}\n";

// Fragment shader source code
const char* fragment_shader_source =
    "precision mediump float;\n"
    "uniform vec4 color;\n"
    "void main() {\n"
    "    gl_FragColor = color;\n"
    "}\n";

GLuint create_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for shader compile errors
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        char* log = (char*) malloc(log_length);
        glGetShaderInfoLog(shader, log_length, NULL, log);
        fprintf(stderr, "Shader compilation failed: %s\n", log);
        free(log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void locus_init_gl() {
    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Use the shader program
    glUseProgram(shader_program);

    // Get attribute and uniform locations
    position_attr = glGetAttribLocation(shader_program, "position");
    color_uniform = glGetUniformLocation(shader_program, "color");

    // Set up OpenGL settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void locus_draw_rectangle(float x, float y, float width, float height, float red, float green, float blue, float alpha) {
    glUseProgram(shader_program);

    // Define rectangle vertices
    GLfloat vertices[] = {
        x, y,             // Bottom left
        x + width, y,    // Bottom right
        x, y + height,   // Top left
        x + width, y + height  // Top right
    };

    // Set the color uniform
    glUniform4f(color_uniform, red, green, blue, alpha);

    // Pass the vertex data to the shader
    glVertexAttribPointer(position_attr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(position_attr);

    // Draw the rectangle as two triangles (triangle strip)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Disable the vertex attribute
    glDisableVertexAttribArray(position_attr);
}
