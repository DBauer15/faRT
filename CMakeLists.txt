cmake_minimum_required(VERSION 3.22)
project(fart)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#if (APPLE)
    #set(CMAKE_OSX_ARCHITECTURES "x86_64")
#endif()

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Compile options
option(BUILD_OPENGL_RENDERER "Build the OpenGL renderer." OFF)
option(BUILD_METAL_RENDERER "Build the Metal renderer." OFF)

# Write all binaries directly to the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(dependencies)
add_subdirectory(src)
