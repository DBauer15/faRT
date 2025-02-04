#include "shader.h"

namespace fart
{

Shader::Shader(WGPUDevice device,
        const char* source,
        std::string entrypoint) {
    m_entrypoint = entrypoint;

    WGPUShaderModuleWGSLDescriptor shader_code_descriptor = {};
    shader_code_descriptor.chain.next = nullptr;
    shader_code_descriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shader_code_descriptor.code = source;

    WGPUShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.nextInChain = &shader_code_descriptor.chain;

    m_shader = wgpuDeviceCreateShaderModule(device, &shader_descriptor);
}

Shader::~Shader() {
    if (m_shader) {
        wgpuShaderModuleRelease(m_shader);
    }
}
}