if (NOT BUILD_WEBGPU_RENDERER)
    return()
endif()

add_library(renderer_webgpu
    renderer.cpp
    renderer.h
    texture.cpp
    texture.h
)

set(SHADERS ${CMAKE_CURRENT_LIST_DIR}/wgsl/pathtracer.wgsl)
embed_shaders(renderer_webgpu embedded_shaders ${SHADERS} TRUE)

set_target_properties(renderer_webgpu PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_include_directories(renderer_webgpu PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/..)

target_link_libraries(renderer_webgpu PUBLIC 
    glfw
    glm::glm
    stage
    webgpu
    glfw3webgpu
)

target_compile_definitions(renderer_webgpu PUBLIC WEBGPU_RENDERER)