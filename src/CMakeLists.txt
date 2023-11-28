#add_subdirectory(opengl)
add_subdirectory(metal)


add_executable(fart
    common/app.h
    common/app.cpp
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)

target_link_libraries(fart PUBLIC
    glfw
    tinyobjloader
    glm::glm
    renderer_metal
    )

install(TARGETS fart
        RUNTIME DESTINATION bin)
