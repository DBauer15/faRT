#include "texture.h"

namespace fart
{

Texture::Texture(MTL::Device* device,
                 const uint32_t width,
                 const uint32_t height,
                 MTL::PixelFormat format,
                 MTL::StorageMode storage_mode,
                 MTL::ResourceUsage usage)
{
    m_format = format;
    m_storage_mode = storage_mode;
    m_usage = usage;

    resize(device, width, height);
}

Texture::Texture(Texture&& other) {
    m_texture = other.m_texture;
    m_width = other.m_width;
    m_height = other.m_height;
    m_format = other.m_format;
    m_storage_mode = other.m_storage_mode;
    m_usage = other.m_usage;
    other.m_texture = nullptr;
}

Texture&
Texture::operator=(Texture&& other) {
    if (m_texture)
        m_texture->release();
    m_texture = other.m_texture;
    m_width = other.m_width;
    m_height = other.m_height;
    m_format = other.m_format;
    m_storage_mode = other.m_storage_mode;
    m_usage = other.m_usage;
    other.m_texture = nullptr;
    
    return *this;
}

Texture::~Texture() {
    if (m_texture)
        m_texture->release();
}

void
Texture::setData(uint8_t* data) {
    MTL::Region region = MTL::Region(0, 0, m_width, m_height);
    m_texture->replaceRegion(region, 0, (const void*) data, m_width * getSizeOfPixelFormat());
}

void
Texture::resize(MTL::Device* device, 
                const uint32_t width,
                const uint32_t height)
{
    if (width == m_width && height == m_height) return;
    m_width = width;
    m_height = height;

    MTL::TextureDescriptor *texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    texture_descriptor->setPixelFormat(m_format);
    texture_descriptor->setTextureType(MTL::TextureType2D);
    texture_descriptor->setWidth(m_width);
    texture_descriptor->setHeight(m_height);

    texture_descriptor->setUsage(m_usage);
    texture_descriptor->setStorageMode(m_storage_mode);

    if (m_texture)
        m_texture->release();
    m_texture = device->newTexture(texture_descriptor);

    texture_descriptor->release();
}

void
Texture::clear() {
}

size_t
Texture::getSizeOfPixelFormat() {
    switch(m_format) {
        case MTL::PixelFormatRGBA32Float:
        case MTL::PixelFormatRGBA32Sint:
        case MTL::PixelFormatRGBA32Uint:
            return 16;
        case MTL::PixelFormatRGBA16Float:
        case MTL::PixelFormatRGBA16Sint:
        case MTL::PixelFormatRGBA16Snorm:
        case MTL::PixelFormatRGBA16Uint:
        case MTL::PixelFormatRGBA16Unorm:
            return 8;
        case MTL::PixelFormatRGBA8Sint:
        case MTL::PixelFormatRGBA8Snorm:
        case MTL::PixelFormatRGBA8Uint:
        case MTL::PixelFormatRGBA8Unorm:
        case MTL::PixelFormatRGBA8Unorm_sRGB:
            return 4;
        default:
            throw std::runtime_error("Unsupported texture pixel format");
    }
}
    
}
