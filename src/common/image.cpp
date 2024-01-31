#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace fart {

Image::Image(std::string filename) {
    stbi_set_flip_vertically_on_load(true);  
    uint8_t* img = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 4);
    //m_image = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 4);

    m_image = img;
    if (m_image == nullptr) {
        ERR("Unable to load image '" + filename + "'");
        return;
    }
    //for (int i = 0; i < getWidth() * getHeight() * getChannels(); i++) {
        //std::cout << std::to_string(m_image[i]) << " ";
    //}
}

Image::~Image() {
    //if (m_image != nullptr)
        //stbi_image_free(m_image);
}

}
