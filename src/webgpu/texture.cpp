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

Texture::~Texture() {
    if (m_texture_view) {
        wgpuTextureViewRelease(m_texture_view);
    }
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