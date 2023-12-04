#pragma once
#include <string>

#include "gldefs.h"
#include "common/defs.h"

#define MAKE_UNIFORM_SETTER(fn_name, TC, T, program) \
    void fn_name (const std::string &name, T* value) { \
        glUniform##TC(glGetUniformLocation(program, name.c_str()), 1, value); \
    }

#define MAKE_UNIFORM_MATRIX_SETTER(fn_name, TC, T, program) \
    void fn_name (const std::string &name, T* value, bool transpose = false) { \
        glUniformMatrix##TC(glGetUniformLocation(program, name.c_str()), 1, transpose, value); \
    }

namespace fart {

struct Shader {

    public:
        Shader(std::string vert_binary_path, std::string frag_binary_path);

        void use();
        void unuse();

        GLuint getUniformLocation(std::string name) const { return glGetUniformLocation(m_program, name.c_str()); }
        GLuint getAttribLocation(std::string name) const { return glGetAttribLocation(m_program, name.c_str()); }

        MAKE_UNIFORM_SETTER(setBool, 1iv, int, m_program);
        MAKE_UNIFORM_SETTER(setInt, 1iv, int, m_program);
        MAKE_UNIFORM_SETTER(setInt2, 2iv, int, m_program);
        MAKE_UNIFORM_SETTER(setInt3, 3iv, int, m_program);
        MAKE_UNIFORM_SETTER(setFloat, 1fv, float, m_program);
        MAKE_UNIFORM_SETTER(setFloat2, 2fv, float, m_program);
        MAKE_UNIFORM_SETTER(setFloat3, 3fv, float, m_program);
        MAKE_UNIFORM_MATRIX_SETTER(setFloat3x3, 3fv, float, m_program);
        MAKE_UNIFORM_MATRIX_SETTER(setFloat4x4, 4fv, float, m_program);

    private:
        GLuint m_program;

        GLuint loadShaderProgram(std::string vert_binary_path, std::string frag_binary_path);
        GLuint loadShaderStage(std::string binary_path, GLenum stage);

};

}
