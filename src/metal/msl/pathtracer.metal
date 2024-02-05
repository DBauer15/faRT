#include <metal_stdlib>
#include "common/types.h"
#include "postprocess.h"

using namespace metal;
using namespace raytracing;

float4 miss(ray ray) {
    float4 sky = float4(70./255., 169./255., 235./255., 1.f);
    float4 haze = float4(127./255., 108./255., 94./255., 1.f);
    float4 background = mix(sky, haze, (ray.direction.y + 1.f) /2.f);
    return background;
}

ray spawnRay(float2 d, Uniforms uniforms) {
    ray ray;
    ray.origin = uniforms.eye;

    ray.direction = normalize(uniforms.dir + 
                              d.x * uniforms.aspect_ratio * cross(uniforms.dir, uniforms.up) +
                              d.y * uniforms.up);
    
    ray.max_distance = INFINITY;

    return ray;
}

kernel void pathtracer(
            uint2                                                  tid                       [[thread_position_in_grid]],
            texture2d<float, access::write>                        dstTex                    [[texture(0)]],
            constant MTLAccelerationStructureInstanceDescriptor   *instances                 [[buffer(0)]],
            instance_acceleration_structure                        accelerationStructure     [[buffer(1)]],
            constant Uniforms&                                     uniforms                  [[buffer(2)]]  
        ) {

        if (tid.x > uniforms.viewport_size.x || tid.y > uniforms.viewport_size.y) return;

        // Create an intersector to test for intersection between the ray and the geometry in the scene.
        intersector<triangle_data, instancing> i;
        i.assume_geometry_type(geometry_type::triangle);
        i.force_opacity(forced_opacity::opaque);
        i.accept_any_intersection(false);

        typename intersector<triangle_data, instancing>::result_type intersection;

        float2 uv = (float2)tid / uniforms.viewport_size;
        uv = uv - 0.5f;
        ray ray = spawnRay(uv, uniforms);

        intersection = i.intersect(ray, accelerationStructure, 0xFF);

        if (intersection.type == intersection_type::none) {
            float4 col = miss(ray);
            dstTex.write(col, tid);
        } else {
            dstTex.write(float4(intersection.distance / uniforms.scene_scale), tid);
        }
}
