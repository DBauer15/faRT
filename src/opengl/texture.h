#pragma once

#include "gldefs.h"
#include "common/defs.h"

namespace fart {

struct Texture {

    public:
        Texture(const uint32_t width, 
                const uint32_t height,
                GLenum internal_format = GL_RGBA32F,
                GLenum src_format = GL_RGBA,
                GLenum src_type = GL_UNSIGNED_BYTE);
        Texture(Texture& other) = delete;
        Texture(Texture&& other);
        Texture& operator=(Texture& other) = delete;
        Texture& operator=(Texture&& other);
        ~Texture();

        void setData(uint8_t* data, 
                     GLenum min_filter = GL_NEAREST, 
                     GLenum mag_filter = GL_LINEAR,
                     GLenum wrap_s = GL_MIRRORED_REPEAT,
                     GLenum wrap_t = GL_MIRRORED_REPEAT);
        void resize(uint32_t width, uint32_t height);
        void clear();

        GLuint& getTexture() { return m_texture; }
        void activate(GLenum texture_unit);
        void bind();
        void unbind();

    private:
        GLuint m_texture {0};

        uint32_t m_width;
        uint32_t m_height;

        GLenum m_internal_format;
        GLenum m_src_format;
        GLenum m_src_type;
};

}
