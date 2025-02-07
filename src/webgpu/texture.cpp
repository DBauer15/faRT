#include "texture.h"

namespace fart
{

Texture::Texture(WGPUSurface surface) {
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(surface, &surface_texture);

    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return;
    }

    m_texture = surface_texture.texture;
    m_owns_texture = false;

    WGPUTextureViewDescriptor view_descriptor;
    view_descriptor.nextInChain = nullptr;
    view_descriptor.label = "Surface texture view";
    view_descriptor.format = wgpuTextureGetFormat(surface_texture.texture);
    view_descriptor.dimension = WGPUTextureViewDimension_2D;
    view_descriptor.baseMipLevel = 0;
    view_descriptor.mipLevelCount = 1;
    view_descriptor.baseArrayLayer = 0;
    view_descriptor.arrayLayerCount = 1;
    view_descriptor.aspect = WGPUTextureAspect_All;
    m_texture_view = wgpuTextureCreateView(surface_texture.texture, &view_descriptor);
}

Texture::Texture(WGPUDevice device,
                const uint32_t width,
                const uint32_t height,
                WGPUTextureFormat format,
                WGPUTextureUsage usage) {

    m_format = format;
    m_usage = usage;

    resize(device, width, height);
}

Texture::Texture(Texture&& other) {
    m_texture = other.m_texture;
    m_format = other.m_format;
    m_usage = other.m_usage;
    m_width = other.m_width;
    m_height = other.m_height;
    
    other.m_texture = nullptr;
}

Texture&
Texture::operator=(Texture&& other) {
    if (m_texture) {
        wgpuTextureRelease(m_texture);
    }
    m_texture = other.m_texture;
    m_format = other.m_format;
    m_usage = other.m_usage;
    m_width = other.m_width;
    m_height = other.m_height; 

    other.m_texture = nullptr;

    return *this;
}

Texture::~Texture() {
    if (m_texture && m_owns_texture) {
        wgpuTextureRelease(m_texture);
    }
    if (m_texture_view) {
        wgpuTextureViewRelease(m_texture_view);
    }
}

void 
Texture::resize(WGPUDevice device, const uint32_t width, const uint32_t height) {
    if (m_width == width && m_height == height) {
        return;
    }
    m_width = width;
    m_height = height;

    if (m_texture) {
        wgpuTextureRelease(m_texture);
    }
    if (m_texture_view) {
        wgpuTextureViewRelease(m_texture_view);
    }

    WGPUTextureDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.format = m_format;
    descriptor.mipLevelCount = 1;
    descriptor.sampleCount = 1;
    descriptor.size = { m_width, m_height, 1 };
    descriptor.usage = m_usage;
    descriptor.viewFormatCount = 0;
    descriptor.viewFormats = nullptr;

    m_texture = wgpuDeviceCreateTexture(device, &descriptor);

    WGPUTextureViewDescriptor view_descriptor = {};
    view_descriptor.aspect = WGPUTextureAspect_All;
    view_descriptor.baseArrayLayer = 0;
    view_descriptor.arrayLayerCount = 1;
    view_descriptor.baseMipLevel = 0;
    view_descriptor.mipLevelCount = 1;
    view_descriptor.dimension = WGPUTextureViewDimension_2D;
    view_descriptor.format = descriptor.format;

    m_texture_view = wgpuTextureCreateView(m_texture, &view_descriptor);
}

WGPUImageCopyTexture 
Texture::getImageCopyTexture() {
    WGPUImageCopyTexture copy_texture = {};
    copy_texture.aspect = WGPUTextureAspect_All;
    copy_texture.mipLevel = 0;
    copy_texture.origin = {0, 0, 0};
    copy_texture.texture = getTexture();

    return copy_texture;
}

}
