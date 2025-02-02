#pragma once

#include <webgpu/webgpu.h>
#include "buffer.h"
#include "shader.h"

namespace fart {

struct Pipeline {
    public:
        Pipeline() = default;

        template<typename T>
        void addBufferBinding(const Buffer<T> &buffer,  
                              uint32_t binding,
                              WGPUBufferBindingType binding_type,
                              WGPUShaderStageFlags visibility) {
            WGPUBindGroupLayoutEntry layout_entry = {};
            layout_entry.binding = binding;
            layout_entry.buffer.type = binding_type;
            layout_entry.visibility = visibility;

            m_bindgroup_layout_entries.push_back(layout_entry);

            WGPUBindGroupEntry bindgroup_entry = {};
            bindgroup_entry.binding = binding;
            bindgroup_entry.buffer = buffer.getBuffer();
            bindgroup_entry.offset = 0;
            bindgroup_entry.size = buffer.getSize();

            m_bindgroup_entries.push_back(bindgroup_entry);
        }
        void addShader(const Shader &shader);
        void commit(WGPUDevice device);

        WGPUComputePipeline getComputePipeline() { return m_compute_pipeline; }
        WGPUBindGroup getBindGroup()             { return m_bindgroup; }

    private:
        WGPUComputePipeline m_compute_pipeline;

        WGPUPipelineLayout m_pipeline_layout;
        WGPUBindGroupLayout m_bindgroup_layout;
        WGPUBindGroup m_bindgroup;

        std::vector<WGPUBindGroupLayoutEntry> m_bindgroup_layout_entries;
        std::vector<WGPUBindGroupEntry> m_bindgroup_entries;

        std::string m_shader_entrypoint;
        WGPUShaderModule m_shader;
};

}