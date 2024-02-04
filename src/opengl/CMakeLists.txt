if (NOT BUILD_OPENGL_RENDERER)
    return()
endif()

find_package(OpenGL REQUIRED)

add_library(renderer_opengl
    aabb.cpp
    aabb.h
    buffer.cpp
    buffer.h
    bvh.cpp
    bvh.h
    framebuffer.cpp
    framebuffer.h
    renderer.cpp
    renderer.h
    shader.cpp
    shader.h
    texture.cpp
    texture.h
    tlas.cpp
    tlas.h
    vertex_array.cpp
    vertex_array.h
    )

set_target_properties(renderer_opengl PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_include_directories(renderer_opengl PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/..)

target_link_libraries(renderer_opengl PUBLIC 
    glm::glm
    glfw
    OpenGL::OpenGL
    glad
    )

# Preprocess shader source and store in binary dir
file(GLOB_RECURSE SHADER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/glsl/*.glsl)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/pathtracer.vert.glsl 
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/glsl/pathtracer.vert.glsl
    DEPENDS ${SHADER_SOURCES}
    COMMAND glslangValidator ${CMAKE_CURRENT_LIST_DIR}/glsl/pathtracer.vert.glsl -E > ${PROJECT_BINARY_DIR}/pathtracer.vert.glsl)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/pathtracer.frag.glsl 
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/glsl/pathtracer.frag.glsl
    DEPENDS ${SHADER_SOURCES}
    COMMAND glslangValidator ${CMAKE_CURRENT_LIST_DIR}/glsl/pathtracer.frag.glsl -E > ${PROJECT_BINARY_DIR}/pathtracer.frag.glsl)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/postprocess.vert.glsl 
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/glsl/postprocess.vert.glsl
    DEPENDS ${SHADER_SOURCES}
    COMMAND glslangValidator ${CMAKE_CURRENT_LIST_DIR}/glsl/postprocess.vert.glsl -E > ${PROJECT_BINARY_DIR}/postprocess.vert.glsl)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/postprocess.frag.glsl 
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/glsl/postprocess.frag.glsl
    DEPENDS ${SHADER_SOURCES}
    COMMAND glslangValidator ${CMAKE_CURRENT_LIST_DIR}/glsl/postprocess.frag.glsl -E > ${PROJECT_BINARY_DIR}/postprocess.frag.glsl)

add_custom_target(renderer_opengl_shaders ALL 
    DEPENDS 
    ${PROJECT_BINARY_DIR}/pathtracer.frag.glsl 
    ${PROJECT_BINARY_DIR}/pathtracer.vert.glsl
    ${PROJECT_BINARY_DIR}/postprocess.frag.glsl 
    ${PROJECT_BINARY_DIR}/postprocess.vert.glsl
    )
