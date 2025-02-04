#pragma once

#include <string>
#include <webgpu/webgpu.h>

namespace fart
{

struct Shader {
    public:
        Shader(WGPUDevice device,
               const char* source);
        ~Shader();

        WGPUShaderModule getShader() const    { return m_shader; }

    private:
        WGPUShaderModule m_shader;
};
    
}
