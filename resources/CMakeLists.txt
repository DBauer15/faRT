# Copy the sky texture to the binary dir
add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/sky.exr
    MAIN_DEPENCY ${CMAKE_CURRENT_LIST_DIR}/sky.exr
    COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_LIST_DIR}/sky.exr
            ${PROJECT_BINARY_DIR}/sky.exr)

add_custom_target(resources ALL
    DEPENDS
    ${PROJECT_BINARY_DIR}/sky.exr)
