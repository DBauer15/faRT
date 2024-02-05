if (NOT BUILD_METAL_RENDERER)
    return()
endif()


add_library(renderer_metal
    mesh.cpp
    mesh.h
    renderer.mm
    renderer.cpp
    renderer.h
    )

set_target_properties(renderer_metal PROPERTIES
CXX_STANDARD 17
CXX_STANDARD_REQUIRED ON)

target_include_directories(renderer_metal PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/..
    ${METAL_CPP_INCLUDE_DIR})

target_link_libraries(renderer_metal PUBLIC
    glm::glm
    glfw
    )

target_link_libraries(renderer_metal PRIVATE "-framework Metal" "-framework QuartzCore" "-framework Foundation")

# Preprocess shader source and store in binary dir
file(GLOB_RECURSE SHADER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/msl/*)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/pathtracer.air
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/msl/pathtracer.metal
    DEPENDS ${SHADER_SOURCES}
    COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_LIST_DIR}/msl/pathtracer.metal -o ${PROJECT_BINARY_DIR}/pathtracer.air)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/default.metallib
    MAIN_DEPENDENCY ${PROJECT_BINARY_DIR}/pathtracer.air
    COMMAND xcrun -sdk macosx metallib ${PROJECT_BINARY_DIR}/pathtracer.air -o ${PROJECT_BINARY_DIR}/default.metallib)

add_custom_target(renderer_metal_shaders ALL
    DEPENDS 
    ${PROJECT_BINARY_DIR}/default.metallib
    )
