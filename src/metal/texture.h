#pragma once

#include <Metal/Metal.hpp>
#include "common/defs.h"

namespace fart
{

struct Texture {

    public:
        Texture(MTL::Device* device,
                const uint32_t width, 
                const uint32_t height,
                MTL::PixelFormat format,
                MTL::StorageMode storage_mode,
                MTL::TextureUsage usage);
        Texture(Texture& other) = delete;
        Texture(Texture&& other);
        Texture& operator=(Texture& other) = delete;
        Texture& operator=(Texture&& other);
        ~Texture();

        void setData(uint8_t* data);
        void resize(MTL::Device* device, const uint32_t width, const uint32_t height);
        void clear();

        MTL::Texture* getTexture() { return m_texture; }

    private:
        size_t getSizeOfPixelFormat();

        MTL::Texture* m_texture     { nullptr };

        uint32_t m_width            { 0 };
        uint32_t m_height           { 0 };

        MTL::PixelFormat m_format;
        MTL::StorageMode m_storage_mode;
        MTL::TextureUsage m_usage;
};

}
