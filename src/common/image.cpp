#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace fart {

Image::Image(std::string filename) {
    stbi_set_flip_vertically_on_load(true);  
    m_image = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 4);
    m_channels = 4;

    // m_image = img;
    if (m_image == nullptr) {
        ERR("Unable to load image '" + filename + "'");
        return;
    }
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
        stbi_image_free(m_image);
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

}
