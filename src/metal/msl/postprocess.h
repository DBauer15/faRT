/*
 * From: Accelerating ray tracing using Metal
 * Reference:
 * https://developer.apple.com/documentation/metal/metal_sample_code_library/accelerating_ray_tracing_using_metal
 */
#ifndef MSL_POSTPROCESS_H
#define MSL_POSTPROCESS_H

#include <metal_stdlib>
using namespace metal;

// Screen filling quad in normalized device coordinates.
constant float2 quadVertices[] = {
    float2(-1, -1),
    float2(-1,  1),
    float2( 1,  1),
    float2(-1, -1),
    float2( 1,  1),
    float2( 1, -1)
};

struct PostprocessVertexOut {
    float4 position [[position]];
    float2 uv;
};

// Simple vertex shader that passes through NDC quad positions.
vertex PostprocessVertexOut postprocessVertex(unsigned short vid [[vertex_id]]) {
    float2 position = quadVertices[vid];

    PostprocessVertexOut out;

    out.position = float4(position, 0, 1);
    out.uv = position * 0.5f + 0.5f;

    return out;
}

// Simple fragment shader that copies a texture and applies a simple tonemapping function.
fragment float4 postprocessFragment(PostprocessVertexOut in [[stage_in]],
                             texture2d<float> tex)
{
    constexpr sampler sam(min_filter::nearest, mag_filter::nearest, mip_filter::none);

    float3 color = tex.sample(sam, in.uv).xyz;

    // Apply a simple tonemapping function to reduce the dynamic range of the
    // input image into a range which the screen can display.
    color = color / (1.0f + color);

    return float4(color, 1.0f);
}

#endif
