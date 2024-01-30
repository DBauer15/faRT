#include "vertex_array.h"

namespace fart {

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_vao);
}

VertexArray::VertexArray(VertexArray&& other) {
    m_vao = other.m_vao;
    other.m_vao = 0;
}

VertexArray&
VertexArray::operator=(VertexArray&& other) {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = other.m_vao;
    other.m_vao = 0;

    return *this;
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_vao);
}

void
VertexArray::bind() {
    glBindVertexArray(m_vao);
}

void
VertexArray::unbind() {
    glBindVertexArray(0);
}

void
VertexArray::addVertexAttribute(const Shader& shader, std::string attribute_name, GLint size, GLenum dtype, GLsizei stride, const void* offset) {
    GLuint location = shader.getAttribLocation(attribute_name); 
    glVertexAttribPointer(location, size, dtype, false, stride, offset); 
    glEnableVertexAttribArray(location);
}


}
