#include "renderer.h"
#include <glfw3webgpu.h>

#include <embedded_shaders.h>

namespace fart {

WebGPURenderer::~WebGPURenderer()
{
    if (m_instance) {
        wgpuInstanceRelease(m_instance);
    }
    if (m_adapter) {
        wgpuAdapterRelease(m_adapter);
    }
    if (m_device) {
        wgpuDeviceRelease(m_device);
    }
    if (m_queue) {
        wgpuQueueRelease(m_queue);
    }
    if (m_surface) {
        wgpuSurfaceUnconfigure(m_surface);
        wgpuSurfaceRelease(m_surface);
    }
}

void 
WebGPURenderer::init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window)
{
    m_scene = scene;
    m_window = window;

    initWebGPU();
    initBuffers();
    initPipeline();
    initBufferData();

}

void 
WebGPURenderer::initWebGPU()
{
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    
    m_instance = wgpuCreateInstance(&desc);
    if (!m_instance)
    {
        ERR("Could not initilzie WebGPU Instance");
        return;
    }

    m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window->getGlfwWindow());

    m_adapter = requestAdapterSync(m_instance);
    if (!m_adapter) 
    {
        ERR("Could not initilzie WebGPU Adapter");
        return;
    }

    m_device = requestDeviceSync(m_adapter);

    if (!m_device) 
    {
        ERR("Could not initilzie WebGPU Device");
        return;
    }

    m_queue = wgpuDeviceGetQueue(m_device);
}

void
WebGPURenderer::initBuffers() {
    // create input buffer
    m_input_buffer = std::make_unique<Buffer<float>>(m_device, 64L, (WGPUBufferUsage)(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst));

    // create output buffer
    m_output_buffer = std::make_unique<Buffer<float>>(m_device, 64L, (WGPUBufferUsage)(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc));

    // create copy buffer
    m_map_buffer = std::make_unique<Buffer<float>>(m_device, 64L, (WGPUBufferUsage)(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead));
}

void
WebGPURenderer::initTextures() {
}


void
WebGPURenderer::initPipeline() {
    // create shader module
    m_pathtracing_shader = std::make_unique<Shader>(m_device, (char*)pathtracer_wgsl, "pathtracer");

    // create pipline
    m_pathtracing_pipeline = std::make_unique<Pipeline>();
    m_pathtracing_pipeline->addBufferBinding(*m_input_buffer, 0, WGPUBufferBindingType_ReadOnlyStorage, WGPUShaderStage_Compute);
    m_pathtracing_pipeline->addBufferBinding(*m_output_buffer, 1, WGPUBufferBindingType_Storage, WGPUShaderStage_Compute);
    m_pathtracing_pipeline->addShader(*m_pathtracing_shader);
    m_pathtracing_pipeline->commit(m_device);
}

void
WebGPURenderer::initBufferData() {
    // fill buffers
    std::vector<float> input(m_input_buffer->getNElements());
    for (int i = 0; i < input.size(); ++i) {
        input[i] = 0.1f * (i+1);
    }

    m_input_buffer->setData(m_device, m_queue, input);
}

void 
WebGPURenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) 
{
    auto t_start = std::chrono::high_resolution_clock::now();

    // Resize framebuffer if needed
    resizeSurface(m_surface, m_adapter, m_device, m_window->getWidth(), m_window->getHeight());

    // Run pathtracing pass
    renderpassPathtracer();

    // Read outputs
    renderpassReadOutputs();

    // Present framebuffer
	wgpuSurfacePresent(m_surface);

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;

}

void WebGPURenderer::renderpassPathtracer()
{ 


    Texture draw_target(m_surface);
    if (!draw_target.getTextureView())
    {
        WARN("Could not get draw target from surface");
        return;
    }

    // Create command encoder for compute pass
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoder_descriptor);

    // Create compute pass
    WGPUComputePassDescriptor computepass_descriptor = {};
    computepass_descriptor.timestampWrites = nullptr;
    computepass_descriptor.nextInChain = nullptr;
    WGPUComputePassEncoder computepass_encoder = wgpuCommandEncoderBeginComputePass(command_encoder, &computepass_descriptor);

    // Configure compute pass
    wgpuComputePassEncoderSetPipeline(computepass_encoder, m_pathtracing_pipeline->getComputePipeline());
    wgpuComputePassEncoderSetBindGroup(computepass_encoder, 0, m_pathtracing_pipeline->getBindGroup(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(computepass_encoder, 2, 1, 1);
    wgpuComputePassEncoderEnd(computepass_encoder);

    // Get ouputs
    wgpuCommandEncoderCopyBufferToBuffer(command_encoder, m_output_buffer->getBuffer(), 0, m_map_buffer->getBuffer(), 0, m_output_buffer->getSize());

    // Submit work
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, nullptr);
    wgpuQueueSubmit(m_queue, 1, &command_buffer);

    // Clean up
    wgpuComputePassEncoderRelease(computepass_encoder);
    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);
}

void
WebGPURenderer::renderpassReadOutputs() {
    mappingdone = false;
    wgpuBufferMapAsync(m_map_buffer->getBuffer(), WGPUMapMode_Read, 0, m_map_buffer->getSize(), 
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            WebGPURenderer* renderer = (WebGPURenderer*) userdata;
            if (status == WGPUBufferMapAsyncStatus_Success) {
                const float* output = (const float*)wgpuBufferGetConstMappedRange(renderer->m_map_buffer->getBuffer(), 0, renderer->m_map_buffer->getSize());

                for (size_t i = 0; i < renderer->m_map_buffer->getNElements(); ++i) {
                    LOG(output[i]);
                }
            }
            wgpuBufferUnmap(renderer->m_map_buffer->getBuffer());
            renderer->mappingdone = true;
        }, this);

    while (!mappingdone) {

    }
}


/* From: https://eliemichel.github.io/LearnWebGPU/getting-started/adapter-and-device/the-adapter.html */
WGPUAdapter 
WebGPURenderer::requestAdapterSync(WGPUInstance instance) 
{
    WGPURequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.compatibleSurface = m_surface;

    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            ERR("Could not get WebGPU adapter");
            ERR(message);
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        &options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.adapter;
}


/* From: https://eliemichel.github.io/LearnWebGPU/getting-started/adapter-and-device/the-adapter.html */
WGPUDevice 
WebGPURenderer::requestDeviceSync(WGPUAdapter adapter) 
{
    WGPUSupportedLimits supported_limits;
    wgpuAdapterGetLimits(m_adapter, &supported_limits);
    WGPURequiredLimits required_limits = {};
	required_limits.limits.maxBindGroups = 2;
	required_limits.limits.maxUniformBuffersPerShaderStage = 2;
	required_limits.limits.maxUniformBufferBindingSize = 64 * sizeof(float);
    required_limits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
	required_limits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
	required_limits.limits.maxBufferSize = 64 * 4;
	required_limits.limits.maxTextureDimension1D = 4096;
	required_limits.limits.maxTextureDimension2D = 4096;
	required_limits.limits.maxTextureDimension3D = 2048; 
	required_limits.limits.maxTextureArrayLayers = 1;
	required_limits.limits.maxSampledTexturesPerShaderStage = 3;
	required_limits.limits.maxSamplersPerShaderStage = 1;
	required_limits.limits.maxInterStageShaderComponents = 17;
	required_limits.limits.maxStorageBuffersPerShaderStage = 2;
	required_limits.limits.maxComputeWorkgroupSizeX = 32;
	required_limits.limits.maxComputeWorkgroupSizeY = 1;
	required_limits.limits.maxComputeWorkgroupSizeZ = 1;
	required_limits.limits.maxComputeInvocationsPerWorkgroup = 32;
	required_limits.limits.maxComputeWorkgroupsPerDimension = 2;
    required_limits.limits.maxStorageBufferBindingSize = 64 * 4;
    required_limits.limits.maxBindingsPerBindGroup = 2;

    WGPUDeviceDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;
    descriptor.label = "WebGPU Device"; 
    descriptor.requiredFeatureCount = 0;
    descriptor.requiredLimits = &required_limits;
    descriptor.defaultQueue.nextInChain = nullptr;
    descriptor.defaultQueue.label = "The default queue";
    descriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void*) {
        WARN("WebGPU device lost!");
        WARN(reason);
        if (message) WARN(message);
    };

    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter,
        &descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.device;
}

void
WebGPURenderer::resizeSurface(WGPUSurface surface, WGPUAdapter adapter, WGPUDevice device, uint32_t width, uint32_t height)
{
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.width = width;
    config.height = height;

    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
    config.format = surfaceFormat;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;

    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &config);
}

}