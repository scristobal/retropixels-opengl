#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

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

/* Load and compile a single shader from file, caller must call glDeleteShader */
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

/* Load, compile, and link a shader program from vertex and fragment shader files, caller must call glDeleteProgram */
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

GLuint load_texture_png(const char* filepath) {
    unsigned char* pixels = NULL;
    int width = 0;
    int height = 0;
    int channels = 0;
    GLuint texture = 0;

    pixels = stbi_load(filepath, &width, &height, &channels, 4);
    if (!pixels) {
        fprintf(stderr, "Failed to load PNG texture (%s): %s\n", filepath, stbi_failure_reason());
        return 0;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    (void)channels;
    stbi_image_free(pixels);

    return texture;
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
    GLuint sprite_vert_buf = 0;
    GLuint sprite_model_transform_buf = 0;
    GLuint sprite_inds_buf = 0;
    GLuint sprite_texture = 0;
    GLuint VAO = 0;
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

    //
    //  0 - - - 1
    //  | A   / |
    //  |   /   |
    //  | /   B |
    //  2 - - - 3
    //
    float sprite_vertices[] = {
    //    x   y   z   u   v
         -1,  1,  0,  0,  0, // 0
          1,  1,  0,  1,  0, // 1
         -1, -1,  0,  0,  1, // 2
          1, -1,  0,  1,  1, // 3
    };
    GLuint sprite_inds[] = {
        0, 2, 1, // A
        1, 2, 3, // B
    };

    // vertex array object
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // vertex buffer object
    glGenBuffers(1, &sprite_vert_buf);
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vert_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sprite_vertices), sprite_vertices, GL_STATIC_DRAW);

    GLuint sprite_coords_loc = 0;
    GLuint sprite_tex_coords_loc = 1;
    GLuint sprite_model_transform_loc = 2;

    glEnableVertexAttribArray(sprite_coords_loc);
    glVertexAttribPointer(sprite_coords_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(sprite_tex_coords_loc);
    glVertexAttribPointer(sprite_tex_coords_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    float identity_matrix[] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glGenBuffers(1, &sprite_model_transform_buf);
    glBindBuffer(GL_ARRAY_BUFFER, sprite_model_transform_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(identity_matrix), identity_matrix, GL_DYNAMIC_DRAW);

    GLuint i;
    for (i = 0; i < 4; i++) {
        GLuint loc = sprite_model_transform_loc + i;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 4 * 4 * sizeof(float), (void*)(i * 4 * sizeof(float)));
        glVertexAttribDivisor(loc, 1);
    }

    glGenBuffers(1, &sprite_inds_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite_inds_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sprite_inds), sprite_inds, GL_STATIC_DRAW);

    sprite_texture = load_texture_png("assets/avatar-1x.png");
    if (!sprite_texture) {
        goto cleanup;
    }

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "albedoMap"), 0);
    glUniformMatrix4fv(glGetUniformLocation(program, "texTransform"), 1, GL_FALSE, identity_matrix);

    glBindVertexArray(0);

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sprite_texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    if (sprite_texture) {
        glDeleteTextures(1, &sprite_texture);
    }

    if (sprite_inds_buf) {
        glDeleteBuffers(1, &sprite_inds_buf);
    }

    if (sprite_model_transform_buf) {
        glDeleteBuffers(1, &sprite_model_transform_buf);
    }

    if (sprite_vert_buf) {
        glDeleteBuffers(1, &sprite_vert_buf);
    }

    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }

    if (program) {
        glDeleteProgram(program);
    }

    if (window) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();

    return exit_code;
}
