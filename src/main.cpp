#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <string>
#include <string_view>
#include <fstream>
#include <print>
#include <expected>
#include <memory>

std::expected<std::string, std::string> read(std::string_view  filepath){
    std::ifstream file{ filepath.data() };

    if (!file.is_open()) {
        return std::unexpected{std::format("Failed to open: {}", filepath)};
    }

    return std::string{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };
}

class Shader {
    GLuint id = 0;

public:
    Shader(GLenum type) : id(glCreateShader(type)) {}

    ~Shader() {
        if (id) glDeleteShader(id);
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept : id(other.id) {
        other.id = 0;
    }

    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            if (id) glDeleteShader(id);
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    GLuint get() const { return id; }

    bool compile(const char* source) {
        glShaderSource(id, 1, &source, nullptr);
        glCompileShader(id);

        GLint success;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(id, 512, nullptr, log);
            std::println(stderr, "Shader compilation failed: {}", log);
        }
        return success;
    }
};

class Program {
    GLuint id = 0;

public:
    Program() : id(glCreateProgram()) {}

    ~Program() {
        if (id) glDeleteProgram(id);
    }

    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    Program(Program&& other) noexcept : id(other.id) {
        other.id = 0;
    }

    Program& operator=(Program&& other) noexcept {
        if (this != &other) {
            if (id) glDeleteProgram(id);
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    GLuint get() const { return id; }

    void attach(const Shader& shader) {
        glAttachShader(id, shader.get());
    }

    bool link() {
        glLinkProgram(id);

        GLint success;
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            glGetProgramInfoLog(id, 512, nullptr, log);
            std::println(stderr, "Program linking failed: {}", log);
        }
        return success;
    }

    void use() {
        glUseProgram(id);
    }
};



struct GLFWwindow_deleter {
    void operator()(GLFWwindow* w) {
        if(w) glfwDestroyWindow(w);
    }
};


struct GLFW_cleanup {
    ~GLFW_cleanup() { 
        glfwTerminate(); 
    }
};


int main() {
    GLFW_cleanup glfw_guard;

    glfwSetErrorCallback([](int error, const char* desc) {
        std::println(stderr, "GLFW Error {}: {}", error, desc);
    });

    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::unique_ptr<GLFWwindow, GLFWwindow_deleter> window {
        glfwCreateWindow(640, 480, "RetroPixel", nullptr, nullptr)
    };

    if (!window) {
        return EXIT_FAILURE;
    }

    glfwSetKeyCallback(window.get(),[](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    });

    glfwMakeContextCurrent(window.get());
    if (!gladLoadGL(glfwGetProcAddress)) {
        return EXIT_FAILURE;
    }

    auto vertex_source = read("shaders/sprite.vertex.glsl");
    if (!vertex_source) {
        std::println(stderr, "Error: {}", vertex_source.error());
        return EXIT_FAILURE;
    }

    Shader vertex_shader(GL_VERTEX_SHADER);
    if (!vertex_shader.compile(vertex_source->c_str())) {
        return EXIT_FAILURE;
    }

    auto fragment_source = read("shaders/sprite.fragment.glsl");
    if (!fragment_source) {
        std::println(stderr, "Error: {}", fragment_source.error());
        return EXIT_FAILURE;
    }

    Shader fragment_shader(GL_FRAGMENT_SHADER);
    if (!fragment_shader.compile(fragment_source->c_str())) {
        return EXIT_FAILURE;
    }

    Program program;
    program.attach(vertex_shader);
    program.attach(fragment_shader);
    if (!program.link()) {
        return EXIT_FAILURE;
    }

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window.get())) {
        int width, height;
        glfwGetFramebufferSize(window.get(), &width, &height);

        glViewport(0,0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window.get());
        glfwPollEvents();
    }
}


