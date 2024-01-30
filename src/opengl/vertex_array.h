#pragma once

#include "gldefs.h"
#include "common/defs.h"

#include "shader.h"
#include "buffer.h"

namespace fart {

struct VertexArray {

    public:
        VertexArray();
        VertexArray(VertexArray& other) = delete;
        VertexArray(VertexArray&& other);
        VertexArray& operator=(VertexArray& other) = delete;
        VertexArray& operator=(VertexArray&& other);
        ~VertexArray();

        void addVertexAttribute(const Shader& shader, std::string attribute_name, GLint size, GLenum dtype, GLsizei stride, const void* offset = 0);

        void bind();
        void unbind();

    private:
        GLuint m_vao;

};

}
