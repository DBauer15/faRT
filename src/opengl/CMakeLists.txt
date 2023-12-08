if (NOT BUILD_OPENGL_RENDERER)
    return()
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(renderer_opengl
    buffer.cpp
    buffer.h
    bvh.cpp
    bvh.h
    renderer.cpp
    renderer.h
    shader.cpp
    shader.h
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
    GLEW::GLEW
    )

# Compile and copy SPIR-V shaders
#add_custom_command(TARGET renderer_opengl POST_BUILD
    #MAIN_DEPENDENCY triangle.vert.glsl
    #COMMAND glslc -fshader-stage=vert ${CMAKE_CURRENT_LIST_DIR}/triangle.vert.glsl -o ${PROJECT_BINARY_DIR}/triangle.vert.spv)

#add_custom_command(TARGET renderer_opengl POST_BUILD
    #MAIN_DEPENDENCY triangle.frag.glsl
    #COMMAND glslc -fshader-stage=frag ${CMAKE_CURRENT_LIST_DIR}/triangle.frag.glsl -o ${PROJECT_BINARY_DIR}/triangle.frag.spv)

# Copy GLSL shader source
file(GLOB SHADER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.glsl)
file(COPY ${SHADER_SOURCES} DESTINATION ${PROJECT_BINARY_DIR})
