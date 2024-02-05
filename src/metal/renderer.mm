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

}
