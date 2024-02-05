#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <glm/glm.hpp>
#include <string>
#include <chrono>
#include "renderer.h"

namespace fart {

MetalRenderer::~MetalRenderer() {
    if (m_device)
        m_device->release();
    if (m_command_queue)
        m_command_queue->release();
    if (m_pathtracing_pipeline_state)
        m_pathtracing_pipeline_state->release();
    if (m_postprocess_pipeline_state)
        m_postprocess_pipeline_state->release();
}

void
MetalRenderer::init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) {
    m_scene = scene;
    m_window = window;

    initMetal();
    initFrameBuffer();
    initBuffers();
    initAccelerationStructure();
    initPipeline();
}

void
MetalRenderer::initMetal() {
    m_device = MTL::CreateSystemDefaultDevice();
    m_command_queue = m_device->newCommandQueue();
    m_library = m_device->newDefaultLibrary();
}

void
MetalRenderer::initFrameBuffer() {
    // Attach a Metal layer to the GLFW window
    m_layer = CA::MetalLayer::layer();
    m_layer->setDevice(m_device);
    m_layer->setFramebufferOnly(true);
    m_layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    addLayerToWindow(m_window->getGlfwWindow(), m_layer);

    // Create accumulation textures
    MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    texture_descriptor->setPixelFormat(MTL::PixelFormatRGBA32Float);
    texture_descriptor->setTextureType(MTL::TextureType2D);
    texture_descriptor->setWidth(m_window->getWidth());
    texture_descriptor->setHeight(m_window->getHeight());

    texture_descriptor->setStorageMode(MTL::StorageModePrivate);
    texture_descriptor->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);

    m_accum_texture0 = m_device->newTexture(texture_descriptor);
    m_accum_texture1 = m_device->newTexture(texture_descriptor);
}

void
MetalRenderer::initBuffers() {
    for (auto& object : m_scene->getObjects()) {
        MetalObject metal_object(m_device, object);
        m_objects.push_back(metal_object);
    }

    m_uniforms = m_device->newBuffer(sizeof(MetalRendererUniforms), MTL::StorageModeShared);
}

MTL::AccelerationStructure*
MetalRenderer::createAccelerationStructureWithDescriptor(MTL::AccelerationStructureDescriptor* descriptor) {
// Query for the sizes needed to store and build the acceleration structure.
    MTL::AccelerationStructureSizes accel_sizes = m_device->accelerationStructureSizes(descriptor);

    // Allocate an acceleration structure large enough for this descriptor. This method
    // doesn't actually build the acceleration structure, but rather allocates memory.
    MTL::AccelerationStructure* acceleration_structure = m_device->newAccelerationStructure(accel_sizes.accelerationStructureSize);

    // Allocate scratch space Metal uses to build the acceleration structure.
    // Use MTLResourceStorageModePrivate for the best performance because the sample
    // doesn't need access to buffer's contents.
    MTL::Buffer* scratch_buffer = m_device->newBuffer(accel_sizes.buildScratchBufferSize, MTL::ResourceStorageModePrivate);

    // Create a command buffer that performs the acceleration structure build.
    MTL::CommandBuffer* command_buffer = m_command_queue->commandBuffer();

    // Create an acceleration structure command encoder.
    MTL::AccelerationStructureCommandEncoder* command_encoder = command_buffer->accelerationStructureCommandEncoder();

    // Allocate a buffer for Metal to write the compacted accelerated structure's size into.
    MTL::Buffer* compacted_size_buffer = m_device->newBuffer(sizeof(uint32_t), MTL::ResourceStorageModeShared);

    // Schedule the actual acceleration structure build.
    command_encoder->buildAccelerationStructure(acceleration_structure,
                                                descriptor,
                                                scratch_buffer,
                                                0);

    // Compute and write the compacted acceleration structure size into the buffer. You
    // must already have a built acceleration structure because Metal determines the compacted
    // size based on the final size of the acceleration structure. Compacting an acceleration
    // structure can potentially reclaim significant amounts of memory because Metal must
    // create the initial structure using a conservative approach.
    command_encoder->writeCompactedAccelerationStructureSize(acceleration_structure, compacted_size_buffer, 0);

    // End encoding, and commit the command buffer so the GPU can start building the
    // acceleration structure.
    command_encoder->endEncoding();

    command_buffer->commit();

    // The sample waits for Metal to finish executing the command buffer so that it can
    // read back the compacted size.

    // Note: Don't wait for Metal to finish executing the command buffer if you aren't compacting
    // the acceleration structure, as doing so requires CPU/GPU synchronization. You don't have
    // to compact acceleration structures, but do so when creating large static acceleration
    // structures, such as static scene geometry. Avoid compacting acceleration structures that
    // you rebuild every frame, as the synchronization cost may be significant.

    command_buffer->waitUntilCompleted();

    uint32_t compacted_size = *(uint32_t *)compacted_size_buffer->contents();

    // Allocate a smaller acceleration structure based on the returned size.
    MTL::AccelerationStructure* compacted_accerleration_structure = m_device->newAccelerationStructure(compacted_size);

    // Create another command buffer and encoder.
    command_buffer = m_command_queue->commandBuffer();
    command_encoder = command_buffer->accelerationStructureCommandEncoder();

    // Encode the command to copy and compact the acceleration structure into the
    // smaller acceleration structure.
    command_encoder->copyAndCompactAccelerationStructure(acceleration_structure, compacted_accerleration_structure);

    // End encoding and commit the command buffer. You don't need to wait for Metal to finish
    // executing this command buffer as long as you synchronize any ray-intersection work
    // to run after this command buffer completes. The sample relies on Metal's default
    // dependency tracking on resources to automatically synchronize access to the new
    // compacted acceleration structure.
    command_encoder->endEncoding();
    command_buffer->commit();

    return compacted_accerleration_structure;
}

void
MetalRenderer::initAccelerationStructure() {

    MTL::AccelerationStructureGeometryDescriptor* geometry_descriptor;
    for(auto& object : m_objects) {
        NS::Array* geometry_descriptors = object.getGeometryDescriptors();

        // Create a primitive acceleration structure descriptor to contain the single piece
        // of acceleration structure geometry.
        MTL::PrimitiveAccelerationStructureDescriptor* accel_descriptor = MTL::PrimitiveAccelerationStructureDescriptor::descriptor();
        accel_descriptor->setGeometryDescriptors(geometry_descriptors);

        // Build the acceleration structure.
        MTL::AccelerationStructure* acceleration_structure = createAccelerationStructureWithDescriptor(accel_descriptor);

        // Add the acceleration structure to the array of primitive acceleration structures.
        m_blas_list.push_back(acceleration_structure);
    }

    m_instances = m_device->newBuffer(m_scene->getInstances().size() * sizeof(MTL::AccelerationStructureInstanceDescriptor), MTL::StorageModeShared);
    MTL::AccelerationStructureInstanceDescriptor* instance_descriptors = (MTL::AccelerationStructureInstanceDescriptor*) m_instances->contents();

    for (int i = 0; i < m_scene->getInstances().size(); i++) {
        ObjectInstance& instance = m_scene->getInstances()[i];

        // Map the instance to its acceleration structure.
        instance_descriptors[i].accelerationStructureIndex = instance.object_id;

        // Mark the instance as opaque if it doesn't have an intersection function so that the
        // ray intersector doesn't attempt to execute a function that doesn't exist.
        instance_descriptors[i].options = MTL::AccelerationStructureInstanceOptionOpaque;

        // Set the instance mask, which the sample uses to filter out intersections between rays
        // and geometry. For example, it uses masks to prevent light sources from being visible
        // to secondary rays, which would result in their contribution being double-counted.
        instance_descriptors[i].mask = 0x01;

        // Copy the first three rows of the instance transformation matrix. Metal
        // assumes that the bottom row is (0, 0, 0, 1), which allows the renderer to
        // tightly pack instance descriptors in memory.
        glm::mat4 instance_to_world = glm::inverse(instance.world_to_instance);
        for (int column = 0; column < 4; column++)
            for (int row = 0; row < 3; row++)
                instance_descriptors[i].transformationMatrix.columns[column][row] = instance_to_world[column][row];
    }

    m_instances->didModifyRange(NS::Range(0, m_instances->length()));

    // Create an instance acceleration structure descriptor.
    MTL::InstanceAccelerationStructureDescriptor* accel_descriptor = MTL::InstanceAccelerationStructureDescriptor::descriptor();
    NS::Array* blas_list = NS::Array::array((NS::Object**)&m_blas_list[0], m_blas_list.size());
    accel_descriptor->setInstancedAccelerationStructures(blas_list);
    accel_descriptor->setInstanceCount(m_scene->getInstances().size());
    accel_descriptor->setInstanceDescriptorBuffer(m_instances);

    // Create the instance acceleration structure that contains all instances in the scene.
    m_tlas = createAccelerationStructureWithDescriptor(accel_descriptor);

}

void
MetalRenderer::initPipeline() {
    NS::Error* error;
    
    // Initialize the compute pipeline state that will perform the path tracing step and store the result in a texture
    MTL::Function* pathtracer_function = m_library->newFunction(NS::String::string("pathtracer", NS::StringEncoding::ASCIIStringEncoding));

    MTL::ComputePipelineDescriptor* compute_pipeline_descriptor = MTL::ComputePipelineDescriptor::alloc()->init();
    compute_pipeline_descriptor->setComputeFunction(pathtracer_function);
    compute_pipeline_descriptor->setThreadGroupSizeIsMultipleOfThreadExecutionWidth(true);

    m_pathtracing_pipeline_state = m_device->newComputePipelineState(compute_pipeline_descriptor, 0, nullptr, &error);
    
    if (error)
        ERR("Error compiling postprocessing pipeline");
    
    // Initialize the renderpipelinestate that will copy the path tracer compute output texture to the screen via fullscreen quad rendering
    MTL::Function* vertex_function = m_library->newFunction(NS::String::string("postprocessVertex", NS::StringEncoding::ASCIIStringEncoding));
    MTL::Function* fragment_function= m_library->newFunction(NS::String::string("postprocessFragment", NS::StringEncoding::ASCIIStringEncoding));

    MTL::RenderPipelineDescriptor* render_pipeline_descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    render_pipeline_descriptor->setVertexFunction(vertex_function);
    render_pipeline_descriptor->setFragmentFunction(fragment_function);
    render_pipeline_descriptor->colorAttachments()->object(0)->setPixelFormat((MTL::PixelFormat)m_layer->pixelFormat());

    m_postprocess_pipeline_state = m_device->newRenderPipelineState(render_pipeline_descriptor, &error);

    if (error)
        ERR("Error compiling fullscreen quad render pipeline");

    render_pipeline_descriptor->release();
}

void
MetalRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) {
    auto t_start = std::chrono::high_resolution_clock::now();

    // Update uniforms
    MetalRendererUniforms* uniforms = (MetalRendererUniforms*)m_uniforms->contents();
    uniforms->frame_number += 1;
    uniforms->viewport_size = glm::vec4(m_window->getViewportSize(), 0, 0);
    uniforms->aspect_ratio = (float)m_window->getWidth() / m_window->getHeight();
    uniforms->scene_scale = m_scene->getSceneScale();
    uniforms->eye = glm::vec4(eye, 0);
    uniforms->dir = glm::vec4(dir, 0);
    uniforms->up = glm::vec4(up, 0);
    m_uniforms->didModifyRange(NS::Range(0, sizeof(MetalRendererUniforms)));

    MTL::CommandBuffer* command_buffer = m_command_queue->commandBuffer();
    renderpassPathtracer(command_buffer);
    command_buffer->commit();
    command_buffer->waitUntilCompleted();
    command_buffer->release();
    command_buffer = m_command_queue->commandBuffer();
    renderpassPostprocess(command_buffer);
    command_buffer->commit();
    command_buffer->waitUntilCompleted();
    std::swap(m_accum_texture0, m_accum_texture1);
    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;
}

void
MetalRenderer::renderpassPathtracer(MTL::CommandBuffer* command_buffer) {
    MTL::Size threadsPerThreadgroup = MTL::Size(8, 8, 1);
    MTL::Size threadgroups = MTL::Size((m_window->getWidth()  + threadsPerThreadgroup.width  - 1) / threadsPerThreadgroup.width,
                                       (m_window->getHeight() + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height,
                                       1);
    
    MTL::ComputeCommandEncoder* compute_command_encoder = command_buffer->computeCommandEncoder();
    compute_command_encoder->setComputePipelineState(m_pathtracing_pipeline_state);
    compute_command_encoder->setTexture(m_accum_texture0, 0);
    compute_command_encoder->setBuffer(m_instances, 0, 0);
    compute_command_encoder->setAccelerationStructure(m_tlas, 1);
    compute_command_encoder->setBuffer(m_uniforms, 0, 2);

    // Bindless resources need to be explicitely used
    for (auto& object : m_objects) {
        for (auto &geometry : object.getGeometries())
        {
            for (auto &resource : geometry.getResources())
            {
                compute_command_encoder->useResource(resource, MTL::ResourceUsageRead);
            }
        }
    }

    // Same here, bindless BLASs need to be explicitely used
    for (auto& blas : m_blas_list) {
        compute_command_encoder->useResource(blas, MTL::ResourceUsageRead);
    }

    compute_command_encoder->dispatchThreadgroups(threadgroups, threadsPerThreadgroup);
    compute_command_encoder->endEncoding();
}

void
MetalRenderer::renderpassPostprocess(MTL::CommandBuffer* command_buffer) {
    CA::MetalDrawable* drawable = m_layer->nextDrawable();

    MTL::RenderPassDescriptor* render_pass_descriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cad = render_pass_descriptor->colorAttachments()->object(0);

    cad->setTexture(drawable->texture());
    cad->setLoadAction(MTL::LoadActionClear);
    cad->setClearColor(MTL::ClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    cad->setStoreAction(MTL::StoreActionStore);

    MTL::RenderCommandEncoder* render_command_encoder = command_buffer->renderCommandEncoder(render_pass_descriptor);
    render_command_encoder->setRenderPipelineState(m_postprocess_pipeline_state);
    render_command_encoder->setFragmentTexture(m_accum_texture0, 0);

    MTL::PrimitiveType triangle = MTL::PrimitiveType::PrimitiveTypeTriangle;
    NS::UInteger start = 0;
    NS::UInteger count = 6;

    render_command_encoder->drawPrimitives(triangle, start, count);
    render_command_encoder->endEncoding();

    command_buffer->presentDrawable(drawable);

    render_pass_descriptor->release();
    drawable->release();
}

}
