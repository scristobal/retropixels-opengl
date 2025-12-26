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

/* Load and compile a single shader from file */
GLuint compile_shader(GLenum shader_type, const char* filepath) {
    GLuint shader = 0;
    char* source = NULL;
    GLint success;

    source = read_file(filepath);
    if (!source) {
        return 0;
    }

    shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, (const char**)&source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader compilation failed (%s): %s\n", filepath, log);
        glDeleteShader(shader);
        shader = 0;
    }

    free(source);
    return shader;
}

/* Load, compile, and link a shader program from vertex and fragment shader files */
GLuint load_program(const char* vertex_path, const char* fragment_path) {
    GLuint program = 0;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLint success;

    vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_path);
    if (!vertex_shader) {
        goto cleanup;
    }

    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_path);
    if (!fragment_shader) {
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
    }

cleanup:
    if (vertex_shader) glDeleteShader(vertex_shader);
    if (fragment_shader) glDeleteShader(fragment_shader);

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
