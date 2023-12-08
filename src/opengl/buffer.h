#pragma once

#include <cstddef>
#include <vector>
#include <string>

#include "common/defs.h"
#include "gldefs.h"

namespace fart {

struct Buffer {

    public: 
        Buffer(GLenum type, GLenum usage = GL_STATIC_DRAW);
        ~Buffer();

        template <typename T> void setData(std::vector<T>& data) {
            bind();
            GLsizei size = sizeof(T) * data.size(); 
            glBufferData(m_type, size, data.data(), m_usage);
            unbind();

            m_n_elements = data.size();
            m_size = size;
        }
        template <typename T> void setData(T* data, size_t n_elements) {
            bind();
            GLsizei size = sizeof(T) * n_elements;
            glBufferData(m_type, size, data, m_usage);
            unbind();

            m_n_elements = n_elements;
            m_size = size;
        }

        GLuint& getBuffer() { return m_buffer; };
        void bind();
        void unbind();

        size_t getNElements() { return m_n_elements; }
        size_t getSize() { return m_size; }

    protected:
        GLuint m_buffer {0};
        GLenum m_type;
        GLenum m_usage;

        size_t m_n_elements {0};
        size_t m_size {0};
};

struct StorageBuffer : public Buffer {

    public: 
        StorageBuffer(GLuint binding_point) :
            Buffer(GL_SHADER_STORAGE_BUFFER, GL_STATIC_READ),
            m_binding_point(binding_point) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, m_buffer);
        }

        ~StorageBuffer() = default;

    private:
        GLuint m_binding_point {0};
        
};

}
