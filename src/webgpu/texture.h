#pragma once

#include <webgpu/webgpu.h>
#include <common/defs.h>


namespace fart
{

struct Texture {
    public:
        Texture(WGPUSurface surface);
        Texture(WGPUDevice device,
                const uint32_t width,
                const uint32_t height,
                WGPUTextureFormat format,
                WGPUTextureUsage usage);
        Texture(Texture& other) = delete;
        Texture(Texture&& other);
        Texture& operator=(Texture& other) = delete;
        Texture& operator=(Texture&& other);
        ~Texture();

        void resize(WGPUDevice device, const uint32_t width, const uint32_t height);

        WGPUTexture getTexture()                    { return m_texture; }
        WGPUTextureView getTextureView()            { return m_texture_view; }
        WGPUImageCopyTexture getImageCopyTexture();
        
    
    private:
        WGPUTexture m_texture                       { nullptr };
        WGPUTextureView m_texture_view              { nullptr };

        uint32_t m_width        { 0 };
        uint32_t m_height       { 0 };

        WGPUTextureFormat m_format;
        WGPUTextureUsage m_usage;
};

}