#include "pipeline.h"

namespace fart 
{

void 
Pipeline::addTextureBinding(const Texture &texture, uint32_t binding, WGPUTextureSampleType sample_type, WGPUShaderStageFlags visibility) {
    WGPUBindGroupLayoutEntry layout_entry = {};
    layout_entry.binding = binding;
    layout_entry.texture.multisampled = false;
    layout_entry.texture.viewDimension = WGPUTextureViewDimension_2D;
    layout_entry.texture.sampleType = sample_type;
    layout_entry.visibility = visibility;

    m_bindgroup_layout_entries.push_back(layout_entry);

    WGPUBindGroupEntry bindgroup_entry = {};
    bindgroup_entry.binding = binding;
    bindgroup_entry.textureView = texture.getTextureView();

    m_bindgroup_entries.push_back(bindgroup_entry);
}


void
Pipeline::commit(WGPUDevice device) {
    // create bindgroup layout
    WGPUBindGroupLayoutDescriptor bindgroup_layout_descriptor = {};
    bindgroup_layout_descriptor.entries = m_bindgroup_layout_entries.data();
    bindgroup_layout_descriptor.entryCount = m_bindgroup_layout_entries.size();

    m_bindgroup_layout = wgpuDeviceCreateBindGroupLayout(device, &bindgroup_layout_descriptor);

    // create bindgroup
    WGPUBindGroupDescriptor bindgroup_descriptor = {};
    bindgroup_descriptor.entries = m_bindgroup_entries.data();
    bindgroup_descriptor.entryCount = m_bindgroup_entries.size();
    bindgroup_descriptor.layout = m_bindgroup_layout;

    m_bindgroup = wgpuDeviceCreateBindGroup(device, &bindgroup_descriptor);

    // create pipeline layout
    WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = {};
    pipeline_layout_descriptor.bindGroupLayouts = &m_bindgroup_layout;
    pipeline_layout_descriptor.bindGroupLayoutCount = 1;

    m_pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &pipeline_layout_descriptor);
}

void
ComputePipeline::addShader(const Shader &shader, std::string entrypoint) {
    m_shader = shader.getShader();
    m_shader_entrypoint = entrypoint;
}

void
ComputePipeline::commit(WGPUDevice device) {
    Pipeline::commit(device);

    // create pipeline
    WGPUComputePipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.layout = m_pipeline_layout;
    pipeline_descriptor.compute.entryPoint = m_shader_entrypoint.c_str();
    pipeline_descriptor.compute.module = m_shader;

    m_compute_pipeline = wgpuDeviceCreateComputePipeline(device, &pipeline_descriptor);
}

void 
RenderPipeline::addColorTarget(const Texture &target) {
    m_blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	m_blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	m_blend_state.color.operation = WGPUBlendOperation_Add;
	m_blend_state.alpha.srcFactor = WGPUBlendFactor_Zero;
	m_blend_state.alpha.dstFactor = WGPUBlendFactor_One;
	m_blend_state.alpha.operation = WGPUBlendOperation_Add;

    m_target_state.blend = &m_blend_state;
    m_target_state.format = wgpuTextureGetFormat(target.getTexture());
    m_target_state.writeMask = WGPUColorWriteMask_All;

    m_fragment_state.targetCount = 1;
    m_fragment_state.targets = &m_target_state;
}

void 
RenderPipeline::addVertexShader(const Shader &shader, std::string entrypoint) {
    m_vertex_shader = shader.getShader();
    m_vertex_shader_entrypoint = entrypoint;
}

void 
RenderPipeline::addFragmentShader(const Shader &shader, std::string entrypoint) {
    m_fragment_shader = shader.getShader();
    m_fragment_shader_entrypoint = entrypoint;
}

void 
RenderPipeline::addVertexAttribute(uint32_t buffer_id, uint32_t location, WGPUVertexFormat format, uint64_t offset) {
    WGPUVertexAttribute attribute;
    attribute.format = format;
    attribute.shaderLocation = location;
    attribute.offset = offset;

    m_vertex_attributes[buffer_id].push_back(attribute);
}

void
RenderPipeline::commit(WGPUDevice device) {
    Pipeline::commit(device);

    // create pipeline
    WGPURenderPipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.vertex.bufferCount = m_vertex_buffer_layouts.size();
    pipeline_descriptor.vertex.buffers = m_vertex_buffer_layouts.data();
    pipeline_descriptor.vertex.constantCount = 0;
    pipeline_descriptor.vertex.constants = nullptr; /* TODO: Support in the future */
    pipeline_descriptor.vertex.entryPoint = m_vertex_shader_entrypoint.c_str();
    pipeline_descriptor.vertex.module = m_vertex_shader;

    m_fragment_state.constantCount = 0;
    m_fragment_state.constants = nullptr; /* TODO: Support in the future */
    m_fragment_state.entryPoint = m_fragment_shader_entrypoint.c_str();
    m_fragment_state.module = m_fragment_shader;

    pipeline_descriptor.fragment = &m_fragment_state;
    pipeline_descriptor.layout = m_pipeline_layout;

    pipeline_descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeline_descriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pipeline_descriptor.primitive.frontFace = WGPUFrontFace_CCW;
	pipeline_descriptor.primitive.cullMode = WGPUCullMode_None;

    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.multisample.mask = ~0u;

    m_render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_descriptor);
}

}