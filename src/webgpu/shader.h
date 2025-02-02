#pragma once

#include <string>
#include <webgpu/webgpu.h>

namespace fart
{

struct Shader {
    public:
        Shader(WGPUDevice device,
               const char* source,
               std::string entrypoint = "");
        ~Shader();

        WGPUShaderModule getShader() const    { return m_shader; }
        std::string getEntrypoint() const     { return m_entrypoint; }

    private:
        WGPUShaderModule m_shader;
        std::string m_entrypoint;
};
    
}
