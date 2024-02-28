#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "defs.h"

namespace fart {

struct Image {

    public:
        Image(std::string filename);
        Image(glm::vec3 color);
        Image(Image& other) = delete;
        Image(Image&& other);
        Image& operator=(Image& other) = delete;
        Image& operator=(Image&& other);
        ~Image();

        uint8_t* getData() { return m_image; }
        uint32_t getWidth() { return m_width; }
        uint32_t getHeight() { return m_height; }
        uint32_t getChannels() { return m_channels; }

        void scale(glm::vec3 scale);
        void scale(Image& other);

        void mix(glm::vec3 color, glm::vec3 amount);
        void mix(Image& other, glm::vec3 amount);

        bool isValid() { return m_image != nullptr; }

    private:
        uint8_t* m_image { nullptr };

        int32_t m_width;
        int32_t m_height;
        int32_t m_channels;

};

}
