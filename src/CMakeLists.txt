add_subdirectory(opengl)
add_subdirectory(metal)

find_package(TBB REQUIRED)

add_executable(fart
    common/app.h
    common/app.cpp
    common/camera.h
    common/camera.cpp
    common/image.h
    common/image.cpp
    common/mesh.h
    common/scene.h
    common/scene.cpp
    common/window.h
    common/window.cpp
    main.cpp)

set_target_properties(fart PROPERTIES 
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(fart PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    ${STB_INCLUDE_DIR}
    ${PBRT_PARSER_INCLUDE_DIR})

target_link_libraries(fart PUBLIC
    glfw
    tinyobjloader
    pbrtParser
    glm::glm
    TBB::tbb
    )

if (BUILD_OPENGL_RENDERER)
    target_link_libraries(fart PUBLIC renderer_opengl)
    target_compile_definitions(fart PUBLIC OPENGL_RENDERER)
elseif (BUILD_METAL_RENDERER)
    target_link_libraries(fart PUBLIC renderer_metal)
    target_compile_definitions(fart PUBLIC METAL_RENDERER)
endif()

install(TARGETS fart
        RUNTIME DESTINATION bin)
