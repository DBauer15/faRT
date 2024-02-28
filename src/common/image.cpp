#include "image.h"

#include <cstdlib>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "defs.h"

namespace fart {

Image::Image(std::string filename) {
    stbi_set_flip_vertically_on_load(true);  
    uint8_t* image = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 4);
    m_channels = 4;

    size_t image_size = sizeof(uint8_t) * m_width * m_height * m_channels;
    m_image = (uint8_t*)std::malloc(image_size);
    std::memcpy(m_image, image, image_size);

    stbi_image_free(image);

    if (m_image == nullptr) {
        ERR("Unable to load image '" + filename + "'");
        return;
    }
}

Image::Image(glm::vec3 color) {
    m_image = (uint8_t*)std::malloc(sizeof(uint8_t) * 4);
    m_image[0] = color.x * 255;
    m_image[1] = color.y * 255;
    m_image[2] = color.z * 255;
    m_image[3] = 255;

    m_width = 1;
    m_height = 1;
    m_channels = 4;
}

Image::Image(Image&& other) {
    m_image = other.m_image;
    m_width = other.m_width;
    m_height = other.m_height;
    m_channels = other.m_channels;
    other.m_image = nullptr;
}

Image&
Image::operator=(Image&& other) {
    if (m_image != nullptr)
        std::free(m_image);
    m_image = other.m_image;
    m_width = other.m_width;
    m_height = other.m_height;
    m_channels = other.m_channels;
    other.m_image = nullptr;

    return *this;
}

Image::~Image() {
    if (m_image != nullptr)
        stbi_image_free(m_image);
}

void
Image::scale(glm::vec3 scale) {
    if (!isValid()) return;
    for (int i = 0; i < m_width * m_height * m_channels; i++) {
        int channel = i % m_channels;
        if (channel == m_channels-1) continue; // Don't scale alpha
        m_image[i] *= scale[channel];
    }
}

void
Image::scale(Image& other) {
    if (!isValid() || !other.isValid()) return;
    if (m_width != other.getWidth() || m_height != other.getHeight() || m_channels != other.getChannels()) {
        WARN("Cannot scale image with another image of different dimensions");
        return;
    }
    
    for (int i = 0; i < m_width * m_height * m_channels; i++) {
        m_image[i] *= other.getData()[i];
    }
}

void
Image::mix(glm::vec3 color, glm::vec3 amount) {
    if (!isValid()) return;
    for (int i = 0; i < m_width * m_height * m_channels; i++) {
        int channel = i % m_channels;
        if (channel == m_channels-1) continue; // Don't scale alpha
        m_image[i] = m_image[i] * (1.f - amount[channel]) + color[channel] * amount[channel];
    }
}

void
Image::mix(Image& other, glm::vec3 amount) {
    if (!isValid() || !other.isValid()) return;
    if (m_width != other.getWidth() || m_height != other.getHeight() || m_channels != other.getChannels()) {
        WARN("Cannot mix image with another image of different dimensions");
        return;
    }
    
    for (int i = 0; i < m_width * m_height * m_channels; i++) {
        int channel = i % m_channels;
        if (channel == m_channels-1) continue; // Don't scale alpha
        m_image[i] = m_image[i] * (1.f - amount[channel]) + other.getData()[i] * amount[channel];
    }
}

}
