#pragma once

#include <webgpu/webgpu.h>
#include <common/defs.h>


namespace fart
{

struct Texture {
    public:
        Texture(WGPUSurface surface);
        ~Texture();

        WGPUTexture getTexture()                    { return m_texture; }
        WGPUTextureView getTextureView()            { return m_texture_view; }
        WGPUImageCopyTexture getImageCopyTexture();
        
    
    private:
        WGPUTexture m_texture                       { nullptr };
        WGPUTextureView m_texture_view              { nullptr };

        uint32_t m_width        { 0 };
        uint32_t m_height       { 0 };
};

}