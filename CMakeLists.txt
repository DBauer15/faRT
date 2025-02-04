cmake_minimum_required(VERSION 3.22)
project(fart)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#if (APPLE)
    #set(CMAKE_OSX_ARCHITECTURES "x86_64")
#endif()

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    if (EMSCRIPTEN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=sign-conversion -Wno-error=unsafe-buffer-usage-in-libc-call -Wno-error=unsafe-buffer-usage")
    endif()
endif()

# Compile options
if (NOT EMSCRIPTEN)
    option(BUILD_OPENGL_RENDERER "Build the OpenGL renderer." OFF)
    option(BUILD_METAL_RENDERER "Build the Metal renderer." OFF)
endif()
option(BUILD_WEBGPU_RENDERER "Build the WebGPU renderer." OFF)

# Write all binaries directly to the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(dependencies)
add_subdirectory(resources)
add_subdirectory(src)
