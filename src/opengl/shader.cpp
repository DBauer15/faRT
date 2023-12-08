#include "shader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>


namespace fart {

Shader::Shader(std::string vert_binary_path, std::string frag_binary_path) {
    m_program = loadShaderProgram(vert_binary_path, frag_binary_path);
}

void
Shader::use() {
    glUseProgram(m_program);
}

void
Shader::unuse() {
    glUseProgram(0);
}

GLuint
Shader::loadShaderProgram(std::string vert_binary_path, std::string frag_binary_path) {
    GLuint vert_shader = loadShaderStage(vert_binary_path, GL_VERTEX_SHADER);
    GLuint frag_shader = loadShaderStage(frag_binary_path, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);

    // Check status
    GLint status;
    glGetShaderiv(program, GL_LINK_STATUS, &status);

    if (!status) {
        ERR("Failed to link shader program.");
        return 0;
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    SUCC("Linked shader program");

    return program;
}

GLuint
Shader::loadShaderStage(std::string binary_path, GLenum stage) {

    // SPIR-V Compile
    //GLuint shader = glCreateShader(stage);
    
    //// Load shader binary
    //std::ifstream shader_stream(binary_path, std::ios::binary);
    //if (!shader_stream.is_open() || !shader_stream) {
        //ERR("Could not open shader binary: " +binary_path);
    //}

    //std::istreambuf_iterator<char> startit(shader_stream), endit;
    //std::vector<char> shader_buffer(startit, endit);
    //shader_stream.close();

    //// Load using glShaderBinary
    //glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shader_buffer.data(), shader_buffer.size());

    //// Specialize the shader 
    //glSpecializeShader(shader, "main", 0, nullptr, nullptr);

    
    // GLSL Compile
    GLuint shader = glCreateShader(stage);

    std::ifstream shader_stream(binary_path);
    std::stringstream shader_sstream;
    shader_sstream << shader_stream.rdbuf();
    shader_stream.close();

    std::string shader_code = shader_sstream.str();
    const char* shader_code_ptr = shader_code.c_str();

    // Compile shader
    glShaderSource(shader, 1, &shader_code_ptr, nullptr);
    glCompileShader(shader);

    // Check status
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (!status) {
        ERR("Failed to load shader: " + binary_path);
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        ERR(&errorLog[0]);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return 0;
    } 

    SUCC("Loaded shader " + binary_path);

    return shader;
}

}
