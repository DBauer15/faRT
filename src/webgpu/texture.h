#pragma once

#include <webgpu/webgpu.h>
#include <common/defs.h>


namespace fart
{

struct Texture {
    public:
        Texture(WGPUSurface surface);
        ~Texture();

        void clear();

        WGPUTextureView getTexture() { return m_texture; }
        
    
    private:
        WGPUTextureView m_texture   { nullptr };

        uint32_t m_width        { 0 };
        uint32_t m_height       { 0 };
};

}