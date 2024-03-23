if(BUILD_OPENGL_RENDERER)
    add_subdirectory(glad)
endif()

# GLFW
add_subdirectory(glfw)

# Metal C++ Headers
set(METAL_CPP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/metal-cpp CACHE INTERNAL "")

# Stage
add_subdirectory(stage)
set(STAGE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/stage/src CACHE INTERNAL "")