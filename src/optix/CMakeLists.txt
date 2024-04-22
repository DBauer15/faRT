if (NOT BUILD_OPTIX_RENDERER)
    return()
endif()

find_package(OpenGL REQUIRED)
if (WIN32)
    set(libgl opengl32)
else()
    set(libgl OpenGL::OpenGL)
endif()
find_package(CUDA REQUIRED)
find_package(OptiX REQUIRED)

include_directories(
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/..
    ${STAGE_INCLUDE_DIR}
    ${CUDA_INCLUDE_DIRS}
    ${OptiX_INCLUDE})

cuda_compile_and_embed(embedded_ptx
    cuda/pathtracer.cu)

add_library(renderer_optix
    buffer.h
    renderer.cpp
    renderer.h
    mesh.cpp
    mesh.h
    ${embedded_ptx}
    )

set_target_properties(renderer_optix PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_include_directories(renderer_optix PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/..
    ${STAGE_INCLUDE_DIR}
    ${CUDA_INCLUDE_DIRS}
    ${OptiX_INCLUDE})

target_link_libraries(renderer_optix PUBLIC 
    glm::glm
    glfw
    glad
    stage
    ${libgl}
    cuda
    ${CUDA_LIBRARIES}
    )

target_compile_definitions(renderer_optix PUBLIC OPTIX_RENDERER)
