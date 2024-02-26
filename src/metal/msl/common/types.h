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

struct Vertex {
    packed_float3 position;
    packed_float3 normal;
    packed_float2 uv;
    uint material_id;
};

struct Resource {
    device Vertex* vertices;
    device uint* indices;
};

struct Texture {
    texture2d<float> texture;
};

struct OpenPBRMaterial {
    packed_float3  base_color;
    int   base_color_texid;
    float base_weight;
    float base_roughness;
    float base_metalness;
    
    packed_float3  specular_color;
    float specular_weight;
    float specular_roughness;
    float specular_anisotropy;
    float specular_rotation;
    float specular_ior;
    float specular_ior_level;

    float transmission_weight;

    float geometry_opacity;
    int   geometry_opacity_texid;
};

struct SurfaceInteraction {
    float3 p;
    float3 n;
    float3 w_i;
    float3 w_o;
    float2 uv;
    bool front_face;
    OpenPBRMaterial mat;
    device Texture* textures;
};


struct RNG {
    uint state;
};

#endif
