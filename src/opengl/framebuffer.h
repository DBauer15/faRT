#pragma once

#include "gldefs.h"
#include "common/defs.h"

#include "texture.h"

namespace fart {

struct FrameBuffer {

    public:
        FrameBuffer();
        ~FrameBuffer();

        void addAttachment(Texture& texture, GLenum attachment_point);

        GLuint& getFrameBuffer() { return m_framebuffer; }
        void bind();
        void unbind();

        bool isComplete();

    private:
        GLuint m_framebuffer {0};


};

}
