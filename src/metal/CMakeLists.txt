if (NOT BUILD_METAL_RENDERER)
    return()
endif()


add_library(renderer_metal
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


add_custom_command(TARGET renderer_metal POST_BUILD
  #OUTPUT triangle.air
  MAIN_DEPENDENCY triangle.metal
  COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_LIST_DIR}/triangle.metal -o ${PROJECT_BINARY_DIR}/triangle.air)

add_custom_command(TARGET renderer_metal POST_BUILD
    #OUTPUT triangle.metallib
  MAIN_DEPENDENCY triangle.air
  COMMAND xcrun -sdk macosx metallib ${PROJECT_BINARY_DIR}/triangle.air -o ${PROJECT_BINARY_DIR}/default.metallib)
