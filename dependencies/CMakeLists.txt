if(BUILD_OPENGL_RENDERER)
    add_subdirectory(glad)
endif()

# GLFW
add_subdirectory(glfw)

# GLM
add_subdirectory(glm)

# Metal C++ Headers
set(METAL_CPP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/metal-cpp CACHE INTERNAL "")

# Dawn for WebGPU
if (BUILD_WEBGPU_RENDERER)
    add_subdirectory(webgpu)
    add_subdirectory(glfw3webgpu)
endif()

# Stage
if (BUILD_OPENGL_RENDERER)
    set(STAGE_API_USAGE_OPENGL ON)
elseif (BUILD_METAL_RENDERER)
    set(STAGE_API_USAGE_METAL ON)
endif()
add_subdirectory(stage)
set(STAGE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/stage/src CACHE INTERNAL "")