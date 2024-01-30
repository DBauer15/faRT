#include "framebuffer.h"

namespace fart {

FrameBuffer::FrameBuffer() {
    glGenFramebuffers(1, &m_framebuffer);
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) {
    m_framebuffer = other.m_framebuffer;
    other.m_framebuffer = 0;
}

FrameBuffer&
FrameBuffer::operator=(FrameBuffer&& other) {
    glDeleteFramebuffers(1, &m_framebuffer);
    m_framebuffer = other.m_framebuffer;
    other.m_framebuffer = 0;

    return *this;
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &m_framebuffer);
}

void
FrameBuffer::addAttachment(Texture& texture, GLenum attachment_point) {
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, 
                           attachment_point, 
                           GL_TEXTURE_2D, 
                           texture.getTexture(), 0);
    unbind();
}

void
FrameBuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
}

void
FrameBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool
FrameBuffer::isComplete() {
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

}
