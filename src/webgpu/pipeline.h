#pragma once

#include <unordered_map>

#include <webgpu/webgpu.h>
#include "common/defs.h"
#include "buffer.h"
#include "texture.h"
#include "shader.h"

namespace fart {

struct Pipeline {
    public:
        ~Pipeline();

        template<typename T>
        void setBufferBinding(const Buffer<T> &buffer,  
                              uint32_t binding,
                              WGPUBufferBindingType binding_type,
                              WGPUShaderStageFlags visibility) {
            WGPUBindGroupLayoutEntry layout_entry = {};
            layout_entry.binding = binding;
            layout_entry.buffer.type = binding_type;
            layout_entry.visibility = visibility;

            if (m_bindgroup_layout_entries.size() > binding) {
                m_bindgroup_layout_entries[binding] = layout_entry;
            } else {
                m_bindgroup_layout_entries.push_back(layout_entry);
            }

            WGPUBindGroupEntry bindgroup_entry = {};
            bindgroup_entry.binding = binding;
            bindgroup_entry.buffer = buffer.getBuffer();
            bindgroup_entry.offset = 0;
            bindgroup_entry.size = buffer.getSize();

            if (m_bindgroup_entries.size() > binding) {
                m_bindgroup_entries[binding] = bindgroup_entry;
            } else {
                m_bindgroup_entries.push_back(bindgroup_entry);
            }
        }
        void setTextureBinding(const Texture &texture, 
                               uint32_t binding,
                               WGPUTextureSampleType sample_type,
                               WGPUShaderStageFlags visibility);
        void setStorageTextureBinding(const Texture &texture,
                                      uint32_t binding,
                                      WGPUStorageTextureAccess access,
                                      WGPUShaderStageFlags visibility);

        virtual void commit(WGPUDevice device);

        WGPUBindGroup getBindGroup()             { return m_bindgroup; }

    protected:
        WGPUPipelineLayout m_pipeline_layout    { nullptr };
        WGPUBindGroupLayout m_bindgroup_layout  { nullptr };
        WGPUBindGroup m_bindgroup               { nullptr };

        std::vector<WGPUBindGroupLayoutEntry> m_bindgroup_layout_entries;
        std::vector<WGPUBindGroupEntry> m_bindgroup_entries;
};

struct ComputePipeline : public Pipeline {
    public:
        ~ComputePipeline();

        void setShader(const Shader &shader, std::string entrypoint);

        virtual void commit(WGPUDevice device) override;

        WGPUComputePipeline getComputePipeline() { return m_compute_pipeline; }

    private:
        WGPUComputePipeline m_compute_pipeline  { nullptr };
        WGPUShaderModule m_shader               { nullptr };
        std::string m_shader_entrypoint;
};

struct RenderPipeline : public Pipeline {
    public:
        ~RenderPipeline();

        void setColorTarget(const Texture &target);
        void setVertexShader(const Shader &shader, std::string entrypoint);
        void setFragmentShader(const Shader &shader, std::string entrypoint);
        void setVertexAttribute(uint32_t buffer_id, uint32_t location, WGPUVertexFormat format, uint64_t offset);

        template<typename T>
        void setVertexBuffer(const Buffer<T> &buffer, size_t element_stride) {
            if (m_vertex_attributes.find(m_vertex_buffer_layouts.size()) == m_vertex_attributes.end()) {
                ERR("No attributes configured for buffer " + m_vertex_buffer_layouts.size());
                ERR("Vertex buffer not configured");
                return;
            }
            WGPUVertexBufferLayout buffer_layout = {};
            buffer_layout.arrayStride = element_stride * sizeof(T);
            buffer_layout.attributeCount = m_vertex_attributes[m_vertex_buffer_layouts.size()].size();
            buffer_layout.attributes = m_vertex_attributes[m_vertex_buffer_layouts.size()].data();
            buffer_layout.stepMode = WGPUVertexStepMode_Vertex;

            m_vertex_buffer_layouts.push_back(buffer_layout);
        }

        virtual void commit(WGPUDevice device) override;
        WGPURenderPipeline getRenderPipeline() { return m_render_pipeline; }
    private:
        WGPURenderPipeline m_render_pipeline    { nullptr };
        WGPUFragmentState m_fragment_state      {  };
        WGPUBlendState m_blend_state            {  };
        WGPUColorTargetState m_target_state     {  };

        std::vector<WGPUVertexBufferLayout> m_vertex_buffer_layouts;
        std::unordered_map<uint32_t, std::vector<WGPUVertexAttribute>> m_vertex_attributes;

        std::string m_vertex_shader_entrypoint;
        WGPUShaderModule m_vertex_shader        { nullptr };

        std::string m_fragment_shader_entrypoint;
        WGPUShaderModule m_fragment_shader      { nullptr };
};

}
