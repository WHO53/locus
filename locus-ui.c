#include "locus-ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* vertex_shader_src =
    "attribute vec2 a_position;                   \n"
    "uniform vec2 u_resolution;                   \n"
    "void main() {                                \n"
    "    // Convert from pixel coordinates to NDC \n"
    "    vec2 zeroToOne = a_position / u_resolution;\n"
    "    vec2 zeroToTwo = zeroToOne * 2.0;        \n"
    "    vec2 clipSpace = zeroToTwo - 1.0;        \n"
    "    gl_Position = vec4(clipSpace * vec2(1, -1), 0, 1);\n"
    "}                                            \n";

static const char* fragment_shader_src =
    "precision mediump float;                     \n"
    "uniform vec4 u_color;                        \n"
    "void main() {                                \n"
    "    gl_FragColor = u_color;                  \n"
    "}                                            \n";

static GLuint program;
static GLint position_loc;
static GLint resolution_loc;
static GLint color_loc;

static const GLfloat vertices[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

GLuint create_shader_program(const char* vertex_src, const char* fragment_src) {
    GLint compile_status, link_status;

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        char buffer[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, buffer);
        fprintf(stderr, "Vertex shader compile error: %s\n", buffer);
        exit(EXIT_FAILURE);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        char buffer[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, buffer);
        fprintf(stderr, "Fragment shader compile error: %s\n", buffer);
        exit(EXIT_FAILURE);
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        char buffer[512];
        glGetProgramInfoLog(shader_program, 512, NULL, buffer);
        fprintf(stderr, "Program link error: %s\n", buffer);
        exit(EXIT_FAILURE);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

void locus_setup_gles() {
    program = create_shader_program(vertex_shader_src, fragment_shader_src);
    glUseProgram(program);

    position_loc = glGetAttribLocation(program, "a_position");
    resolution_loc = glGetUniformLocation(program, "u_resolution");
    color_loc = glGetUniformLocation(program, "u_color");

    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
}

void locus_rectangle(float x, float y, float width, float height, 
                    float red, float green, float blue, float alpha) {
    GLfloat resolution[2] = { 800.0f, 600.0f };
    glUniform2fv(resolution_loc, 1, resolution);

    glUniform4f(color_loc, red, green, blue, alpha);

    GLfloat rect_vertices[] = {
        x, y,
        x + width, y,
        x, y + height,
        x + width, y + height
    };

    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, rect_vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void locus_cleanup_gles() {
    glDisableVertexAttribArray(position_loc);
    glDeleteProgram(program);
}
