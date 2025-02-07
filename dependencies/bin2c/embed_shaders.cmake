function(embed_shaders TARGET OUTPUT_NAME INPUT_FILES INPUT_IS_ASCII)
    string(REPLACE "," ";" INPUT_LIST ${INPUT_FILES})

    set(OUTPUT_NAME ${CMAKE_BINARY_DIR}/generated_sources/${OUTPUT_NAME})
    set(bin2c_cmd -DOUTPUT_NAME=${OUTPUT_NAME}
                  -DINPUT_FILES=${INPUT_FILES}
                  -DINPUT_IS_ASCII=${INPUT_IS_ASCII}
                  -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin2c.cmake")
    
    add_custom_command(
        OUTPUT ${OUTPUT_NAME}.c ${OUTPUT_NAME}.h
        COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmd}
        DEPENDS ${INPUT_LIST}
        COMMENT "Embedding ${INPUT_LIST} as ${OUTPUT_NAME}.c"
        PRE_BUILD VERBATIM)

    add_custom_target(${TARGET}_SHADERS_ ALL 
        DEPENDS
        ${OUTPUT_NAME}.c
        ${OUTPUT_NAME}.h
    )
    add_dependencies(${TARGET} ${TARGET}_SHADERS_)
    target_sources(${TARGET} PRIVATE ${OUTPUT_NAME}.c)
    target_include_directories(${TARGET} PRIVATE ${CMAKE_BINARY_DIR}/generated_sources)

endfunction()
