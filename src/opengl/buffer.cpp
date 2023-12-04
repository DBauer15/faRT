#include "buffer.h"

namespace fart {

Buffer::Buffer(GLenum type, GLenum usage) : m_type(type), m_usage(usage) {
    glGenBuffers(1, &m_buffer);
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
