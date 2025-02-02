#include "pipeline.h"

namespace fart 
{

void
Pipeline::addShader(const Shader &shader) {
    m_shader = shader.getShader();
    m_shader_entrypoint = shader.getEntrypoint();
}

void
Pipeline::commit(WGPUDevice device) {
    // create bindgroup layout
    WGPUBindGroupLayoutDescriptor bindgroup_layout_descriptor;
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

    // create pipeline
    WGPUComputePipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.layout = m_pipeline_layout;
    pipeline_descriptor.compute.entryPoint = m_shader_entrypoint.c_str();
    pipeline_descriptor.compute.module = m_shader;

    m_compute_pipeline = wgpuDeviceCreateComputePipeline(device, &pipeline_descriptor);
}

}