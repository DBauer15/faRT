#include "renderer.h"
#include <glfw3webgpu.h>

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
WebGPURenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) 
{
    resizeSurface(m_surface, m_adapter, m_device, m_window->getWidth(), m_window->getHeight());

    Texture draw_target(m_surface);
    if (!draw_target.getTexture()) {
        WARN("Could not get draw target from surface");
        return;
    }

    // Create a command encoder for the draw call
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My command encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

	// Create the render pass that clears the screen with our color
	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

	// The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = draw_target.getTexture();
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	// Finally encode and submit the render pass
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	cmdBufferDescriptor.label = "Command buffer";
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder);

	wgpuQueueSubmit(m_queue, 1, &command);
	wgpuCommandBufferRelease(command);

	wgpuSurfacePresent(m_surface);

#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(m_device);
#elif defined(WEBGPU_BACKEND_WGPU)
	// wgpuDevicePoll(m_device, false, nullptr);
#endif

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

    // We wait until userData.requestEnded gets true
    // [...] Wait for request to end

    assert(userData.requestEnded);

    return userData.adapter;
}


WGPUDevice 
WebGPURenderer::requestDeviceSync(WGPUAdapter adapter) 
{
    WGPUDeviceDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;
    descriptor.label = "WebGPU Device"; 
    descriptor.requiredFeatureCount = 0;
    descriptor.requiredLimits = nullptr;
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
    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;

    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &config);
}

}