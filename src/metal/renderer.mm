#include "renderer.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#import <QuartzCore/CAMetalLayer.h>

namespace fart {

void
MetalRenderer::addLayerToWindow(GLFWwindow* window, CA::MetalLayer* layer) {
    NSWindow* cocoa_window = glfwGetCocoaWindow(window);
    CAMetalLayer* native_layer = (__bridge CAMetalLayer*)layer;
    [[cocoa_window contentView] setLayer:native_layer];
    [native_layer setMaximumDrawableCount:3];
    [native_layer setDisplaySyncEnabled:false];
    [[cocoa_window contentView] setWantsLayer:YES];
    [[cocoa_window contentView] setNeedsLayout:YES];
}

void
MetalRenderer::resizeLayer(CA::MetalLayer* layer, uint32_t width, uint32_t height) {
    CAMetalLayer* native_layer = (__bridge CAMetalLayer*)layer;

    CGSize old_size = [native_layer drawableSize];
    CGSize new_size = { (double)width, (double)height };

    if (old_size.width == new_size.width &&
        old_size.height == new_size.height) 
        return;

    LOG("Resizing");
    [native_layer setDrawableSize: new_size];
}

}
