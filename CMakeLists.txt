cmake_minimum_required(VERSION 3.16)
project(RetroPixels LANGUAGES C CXX)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RetroPixels src/main.cpp)

add_subdirectory(external/glfw)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)



add_library(glad external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)



target_link_libraries(RetroPixels PRIVATE glfw glad)
