#include "buffer.h"

namespace fart {

Buffer::Buffer(GLenum type, GLenum usage) : m_type(type), m_usage(usage) {
    glGenBuffers(1, &m_buffer);
}

Buffer::Buffer(Buffer&& other) {
    m_buffer = other.m_buffer;
    m_type = other.m_type;
    m_usage = other.m_usage;
    m_n_elements = other.m_n_elements;
    m_size = other.m_size;
    other.m_buffer = 0;
}

Buffer&
Buffer::operator=(Buffer&& other) {
    glDeleteBuffers(1, &m_buffer);
    m_buffer = other.m_buffer;
    m_type = other.m_type;
    m_usage = other.m_usage;
    m_n_elements = other.m_n_elements;
    m_size = other.m_size;
    other.m_buffer = 0;

    return *this;
}

Buffer::~Buffer() {
    glDeleteBuffers(1, &m_buffer);
}

void
Buffer::bind() {
    glBindBuffer(m_type, m_buffer);
}

void
Buffer::unbind() {
    glBindBuffer(m_type, 0);
}

}
