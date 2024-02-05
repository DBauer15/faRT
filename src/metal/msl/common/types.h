#ifndef MSL_COMMON_TYPES_H
#define MSL_COMMON_TYPES_H

#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    uint frame_number;
    float scene_scale;
    float aspect_ratio;
    float filler;
    float3 eye;
    float3 dir;
    float3 up;
    float2 viewport_size;
};

#endif