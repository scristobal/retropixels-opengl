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

#define SPRITE_VERTEX_BINDING 0
#define SPRITE_MODEL_BINDING 1
#define SPRITE_POSITION_LOCATION 0
#define SPRITE_TEX_COORD_LOCATION 1
#define SPRITE_MODEL_MATRIX_LOCATION 2
#define SPRITE_TEX_TRANSFORM_LOCATION 0
#define SPRITE_TEXTURE_UNIT 0
#define SPRITE_TEXTURE_WIDTH 32
#define SPRITE_TEXTURE_HEIGHT 32
#define SPRITE_TEXTURE_MIP_LEVELS 6

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

static const char* debug_source_string(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API: return "api";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "window-system";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader-compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "third-party";
        case GL_DEBUG_SOURCE_APPLICATION: return "application";
        case GL_DEBUG_SOURCE_OTHER: return "other";
        default: return "unknown";
    }
}

static const char* debug_type_string(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: return "error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated-behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined-behavior";
        case GL_DEBUG_TYPE_PORTABILITY: return "portability";
        case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
        case GL_DEBUG_TYPE_MARKER: return "marker";
        case GL_DEBUG_TYPE_PUSH_GROUP: return "push-group";
        case GL_DEBUG_TYPE_POP_GROUP: return "pop-group";
        case GL_DEBUG_TYPE_OTHER: return "other";
        default: return "unknown";
    }
}

static const char* debug_severity_string(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: return "high";
        case GL_DEBUG_SEVERITY_MEDIUM: return "medium";
        case GL_DEBUG_SEVERITY_LOW: return "low";
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "notification";
        default: return "unknown";
    }
}

static void APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param) {
    (void)length;
    (void)user_param;

    fprintf(stderr, "OpenGL debug [%s/%s/%s] %u: %s\n",
        debug_source_string(source),
        debug_type_string(type),
        debug_severity_string(severity),
        id,
        message);
}

/* Load a single separable shader stage program, caller must call glDeleteProgram */
GLuint load_shader_stage(GLenum shader_type, const char* filepath) {
    GLuint program = 0;
    char* source = NULL;
    GLint success;

    source = read_file(filepath);
    if (!source) {
        return 0;
    }

    program = glCreateShaderProgramv(shader_type, 1, (const GLchar* const*)&source);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
        fprintf(stderr, "Shader stage program failed (%s): %s\n", filepath, log);
        glDeleteProgram(program);
        program = 0;
    }

    free(source);
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
    GLuint vertex_program = 0;
    GLuint fragment_program = 0;
    GLuint sprite_pipeline = 0;
    GLuint sprite_vert_buf = 0;
    GLuint sprite_model_transform_buf = 0;
    GLuint sprite_inds_buf = 0;
    GLuint sprite_texture = 0;
    GLuint VAO = 0;
    unsigned char* sprite_pixels = NULL;
    int sprite_texture_width = 0;
    int sprite_texture_height = 0;
    int sprite_texture_channels = 0;
    int exit_code = EXIT_FAILURE;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        goto cleanup;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    window = glfwCreateWindow(640, 480, "RetroPixel", NULL, NULL);
    if (!window) {
        goto cleanup;
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        goto cleanup;
    }

    if (!GLAD_GL_VERSION_4_6) {
        fprintf(stderr, "OpenGL 4.6 core is required, but this context does not expose it.\n");
        goto cleanup;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debug_callback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    vertex_program = load_shader_stage(GL_VERTEX_SHADER, "shaders/sprite.vertex.glsl");
    if (!vertex_program) {
        goto cleanup;
    }
    glObjectLabel(GL_PROGRAM, vertex_program, -1, "sprite vertex stage");

    fragment_program = load_shader_stage(GL_FRAGMENT_SHADER, "shaders/sprite.fragment.glsl");
    if (!fragment_program) {
        goto cleanup;
    }
    glObjectLabel(GL_PROGRAM, fragment_program, -1, "sprite fragment stage");

    glCreateProgramPipelines(1, &sprite_pipeline);
    glUseProgramStages(sprite_pipeline, GL_VERTEX_SHADER_BIT, vertex_program);
    glUseProgramStages(sprite_pipeline, GL_FRAGMENT_SHADER_BIT, fragment_program);
    glValidateProgramPipeline(sprite_pipeline);

    GLint success;
    glGetProgramPipelineiv(sprite_pipeline, GL_VALIDATE_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetProgramPipelineInfoLog(sprite_pipeline, (GLsizei)sizeof(log), NULL, log);
        fprintf(stderr, "Program pipeline validation failed: %s\n", log);
        glDeleteProgramPipelines(1, &sprite_pipeline);
        sprite_pipeline = 0;
        goto cleanup;
    }
    glObjectLabel(GL_PROGRAM_PIPELINE, sprite_pipeline, -1, "sprite pipeline");

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

    glCreateVertexArrays(1, &VAO);
    glObjectLabel(GL_VERTEX_ARRAY, VAO, -1, "sprite vertex array");

    glCreateBuffers(1, &sprite_vert_buf);
    glObjectLabel(GL_BUFFER, sprite_vert_buf, -1, "sprite vertices");
    glNamedBufferStorage(sprite_vert_buf, sizeof(sprite_vertices), sprite_vertices, 0);
    glVertexArrayVertexBuffer(VAO, SPRITE_VERTEX_BINDING, sprite_vert_buf, 0, 5 * (GLsizei)sizeof(float));

    glEnableVertexArrayAttrib(VAO, SPRITE_POSITION_LOCATION);
    glVertexArrayAttribFormat(VAO, SPRITE_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, SPRITE_POSITION_LOCATION, SPRITE_VERTEX_BINDING);

    glEnableVertexArrayAttrib(VAO, SPRITE_TEX_COORD_LOCATION);
    glVertexArrayAttribFormat(VAO, SPRITE_TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 3 * (GLuint)sizeof(float));
    glVertexArrayAttribBinding(VAO, SPRITE_TEX_COORD_LOCATION, SPRITE_VERTEX_BINDING);

    float identity_matrix[] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glCreateBuffers(1, &sprite_model_transform_buf);
    glObjectLabel(GL_BUFFER, sprite_model_transform_buf, -1, "sprite model transforms");
    glNamedBufferStorage(sprite_model_transform_buf, sizeof(identity_matrix), identity_matrix, GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(VAO, SPRITE_MODEL_BINDING, sprite_model_transform_buf, 0, 4 * 4 * (GLsizei)sizeof(float));
    glVertexArrayBindingDivisor(VAO, SPRITE_MODEL_BINDING, 1);

    GLuint i;
    for (i = 0; i < 4; i++) {
        GLuint loc = SPRITE_MODEL_MATRIX_LOCATION + i;
        glEnableVertexArrayAttrib(VAO, loc);
        glVertexArrayAttribFormat(VAO, loc, 4, GL_FLOAT, GL_FALSE, i * 4 * (GLuint)sizeof(float));
        glVertexArrayAttribBinding(VAO, loc, SPRITE_MODEL_BINDING);
    }

    glCreateBuffers(1, &sprite_inds_buf);
    glObjectLabel(GL_BUFFER, sprite_inds_buf, -1, "sprite indices");
    glNamedBufferStorage(sprite_inds_buf, sizeof(sprite_inds), sprite_inds, 0);
    glVertexArrayElementBuffer(VAO, sprite_inds_buf);

    sprite_pixels = stbi_load("assets/avatar-1x.png", &sprite_texture_width, &sprite_texture_height, &sprite_texture_channels, 4);
    if (!sprite_pixels) {
        fprintf(stderr, "Failed to load PNG texture (assets/avatar-1x.png): %s\n", stbi_failure_reason());
        goto cleanup;
    }
    if (sprite_texture_width != SPRITE_TEXTURE_WIDTH || sprite_texture_height != SPRITE_TEXTURE_HEIGHT) {
        fprintf(stderr, "Unexpected sprite texture dimensions: got %dx%d, expected %dx%d\n",
            sprite_texture_width,
            sprite_texture_height,
            SPRITE_TEXTURE_WIDTH,
            SPRITE_TEXTURE_HEIGHT);
        goto cleanup;
    }
    (void)sprite_texture_channels;

    glCreateTextures(GL_TEXTURE_2D, 1, &sprite_texture);
    glObjectLabel(GL_TEXTURE, sprite_texture, -1, "sprite texture");
    glTextureStorage2D(sprite_texture, SPRITE_TEXTURE_MIP_LEVELS, GL_RGBA8, SPRITE_TEXTURE_WIDTH, SPRITE_TEXTURE_HEIGHT);
    glTextureSubImage2D(sprite_texture, 0, 0, 0, SPRITE_TEXTURE_WIDTH, SPRITE_TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, sprite_pixels);
    glGenerateTextureMipmap(sprite_texture);
    glTextureParameteri(sprite_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(sprite_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(sprite_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(sprite_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(sprite_pixels);
    sprite_pixels = NULL;

    glProgramUniformMatrix4fv(vertex_program, SPRITE_TEX_TRANSFORM_LOCATION, 1, GL_FALSE, identity_matrix);
    glBindProgramPipeline(sprite_pipeline);
    glBindTextureUnit(SPRITE_TEXTURE_UNIT, sprite_texture);
    glBindVertexArray(VAO);

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        const GLfloat clear_color[] = { 0.08f, 0.08f, 0.10f, 1.0f };
        glfwGetFramebufferSize(window, &width, &height);

        glViewportIndexedf(0, 0.0f, 0.0f, (GLfloat)width, (GLfloat)height);
        glClearNamedFramebufferfv(0, GL_COLOR, 0, clear_color);
        glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1, 0, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    if (sprite_pixels) {
        stbi_image_free(sprite_pixels);
    }

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

    if (sprite_pipeline) {
        glDeleteProgramPipelines(1, &sprite_pipeline);
    }

    if (fragment_program) {
        glDeleteProgram(fragment_program);
    }

    if (vertex_program) {
        glDeleteProgram(vertex_program);
    }

    if (window) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();

    return exit_code;
}
