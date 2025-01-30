#include "texture.h"

namespace fart
{

Texture::Texture(WGPUSurface surface) {
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(surface, &surface_texture);

    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return;
    }

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
    m_texture = wgpuTextureCreateView(surface_texture.texture, &view_descriptor);
}

Texture::~Texture() {
    if (m_texture) {
        wgpuTextureViewRelease(m_texture);
    }
}

void
Texture::clear() {

}

}