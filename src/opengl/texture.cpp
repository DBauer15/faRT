#include "texture.h"

namespace fart {

Texture::Texture(const uint32_t width, 
                const uint32_t height,
                GLenum internal_format,
                GLenum src_format,
                GLenum src_type)
    : m_internal_format(internal_format),
      m_src_format(src_format),
      m_src_type(src_type)
{
    glGenTextures(1, &m_texture);
    resize(width, height);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_texture);
}

void
Texture::setData(char* data,
                 GLenum min_filter = GL_NEAREST,
                 GLenum mag_filter = GL_NEAREST) {
    bind();
    glTexImage2D(GL_TEXTURE_2D, 
                 0, m_internal_format,
                 m_width, m_height,
                 0, m_src_format, m_src_type, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    unbind();
}

void 
Texture::resize(uint32_t width, uint32_t height) {
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;

    setData(nullptr);
}

void
Texture::clear() {
    setData(nullptr);
}

void
Texture::activate(GLenum texture_unit) {
    glActiveTexture(texture_unit);
}

void
Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void
Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

}
