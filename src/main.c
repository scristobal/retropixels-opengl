#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Load entire file into memory, caller must free() returned pointer */
char* read_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Failed to allocate %ld bytes of memory when reading: %s\n", size + 1, filepath);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    fclose(file);
    return buffer;
}

/* Load, compile, and link a shader program from vertex and fragment shader files */
GLuint load_program(const char* vertex_path, const char* fragment_path) {
    GLuint program = 0;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    char* vertex_source = NULL;
    char* fragment_source = NULL;
    GLint success;

    vertex_source = read_file(vertex_path);
    if (!vertex_source) {
        goto cleanup;
    }

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const char**)&vertex_source, NULL);
    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, log);
        fprintf(stderr, "Vertex shader compilation failed (%s): %s\n", vertex_path, log);
        goto cleanup;
    }

    fragment_source = read_file(fragment_path);
    if (!fragment_source) {
        goto cleanup;
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const char**)&fragment_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, log);
        fprintf(stderr, "Fragment shader compilation failed (%s): %s\n", fragment_path, log);
        goto cleanup;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Program (%s, %s) linking failed: %s\n", vertex_path, fragment_path, log);
        glDeleteProgram(program);
        program = 0;
        goto cleanup;
    }

cleanup:
    if (vertex_shader) glDeleteShader(vertex_shader);
    if (fragment_shader) glDeleteShader(fragment_shader);
    free(vertex_source);
    free(fragment_source);

    return program;
}

static void error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(void) {
    GLFWwindow* window = NULL;
    GLuint program = 0;
    int exit_code = EXIT_FAILURE;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        goto cleanup;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(640, 480, "RetroPixel", NULL, NULL);
    if (!window) {
        goto cleanup;
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        goto cleanup;
    }

    program = load_program("shaders/sprite.vertex.glsl", "shaders/sprite.fragment.glsl");
    if (!program) {
        goto cleanup;
    }

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    if (program) {
        glDeleteProgram(program);
    }

    if (window) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();

    return exit_code;
}
