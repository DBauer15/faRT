if(BUILD_OPENGL_RENDERER)
    add_subdirectory(glad)
endif()

# GLFW
add_subdirectory(glfw)

# GLM
add_subdirectory(glm)

# Metal C++ Headers
set(METAL_CPP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/metal-cpp CACHE INTERNAL "")

# PBRT Parser
add_subdirectory(pbrt-parser EXCLUDE_FROM_ALL)
set(PBRT_PARSER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/pbrt-parser/pbrtParser/include CACHE INTERNAL "")

# OBJ Parser
add_subdirectory(tinyobjloader)

# Image Processing Library
set(STB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb CACHE INTERNAL "")
