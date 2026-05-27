cmake_minimum_required(VERSION 3.16)
project(RetroPixels LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(RetroPixels src/main.c)

set(RETROPIXELS_SHADERS_DIR "shaders")
set(RETROPIXELS_ASSETS_DIR "assets")

target_compile_definitions(RetroPixels PRIVATE
    RETROPIXELS_SHADERS_DIR="${RETROPIXELS_SHADERS_DIR}"
    RETROPIXELS_ASSETS_DIR="${RETROPIXELS_ASSETS_DIR}"
)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)

add_subdirectory(external/glfw)

add_library(glad external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

target_include_directories(RetroPixels PRIVATE external/stb)
target_link_libraries(RetroPixels PRIVATE glfw glad)
