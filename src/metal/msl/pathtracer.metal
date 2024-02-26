#include <metal_common>
#include <metal_stdlib>
#include "common/types.h"
#include "common/random.h"
#include "common/sampling.h"
#include "common/material.h"
#include "postprocess.h"

#define MIN_RR_DEPTH 3
#define MAX_BOUNCES 5

using namespace metal;
using namespace raytracing;

float3 getNormal(Vertex v0, Vertex v1, Vertex v2, float3 bary) {
    float3 n0 = v0.normal;
    float3 n1 = v1.normal;
    float3 n2 = v2.normal;

    return n0 * bary.x + n1 * bary.y + n2 * bary.z;
}

float2 getUV(Vertex v0, Vertex v1, Vertex v2, float3 bary) {
    float2 uv0 = v0.uv;
    float2 uv1 = v1.uv;
    float2 uv2 = v2.uv;

    return uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;
}

float3 miss(thread ray& ray) {
    float3 sky = float3(70./255., 169./255., 235./255.);
    float3 haze = float3(127./255., 108./255., 94./255.);
    haze = float3(0.85f, 0.3f, 0.1f);
    float3 background = mix(sky, haze, (ray.direction.x + 1.f) / 2.f);
    return background;
}

float4 traceRay(thread ray                                            &ray, 
                instance_acceleration_structure                        acceleration_structure,
                intersection_function_table<triangle_data, instancing> intersection_function_table,
                constant Uniforms                                     &uniforms,
                constant MTLAccelerationStructureInstanceDescriptor   *instances,
                device   Resource                                     *resources,
                constant OpenPBRMaterial                              *materials,
                device   Texture                                      *textures,
                thread RNG                                            &rng)
{
    float3 L = float3(0.f);
    float3 throughput = float3(1.f);

    intersector<triangle_data, instancing> isect;
    isect.assume_geometry_type(geometry_type::triangle);
    isect.accept_any_intersection(false);

    typename intersector<triangle_data, instancing>::result_type intersection;
    
    thread float3 f;
    thread float f_pdf;
    thread SurfaceInteraction si;

    for (int i = 0; i <= MAX_BOUNCES; i++) {
        intersection = isect.intersect(ray, acceleration_structure, 0x01, intersection_function_table);
        if (intersection.type == intersection_type::none) {
            L = throughput * miss(ray);
            break;
        }
                    
        float3 bary = float3(1.f - intersection.triangle_barycentric_coord.x - intersection.triangle_barycentric_coord.y, intersection.triangle_barycentric_coord);
        
        uint instance_index = intersection.instance_id;
        uint geometry_index = intersection.geometry_id;
        uint primitive_index = intersection.primitive_id;

        device Resource & triangle_resource = *(device Resource *)((device char *)resources + sizeof(Resource) * geometry_index);
        Vertex triangle[3];

        triangle[0]  =   triangle_resource.vertices[triangle_resource.indices[primitive_index * 3 + 0]];
        triangle[1]  =   triangle_resource.vertices[triangle_resource.indices[primitive_index * 3 + 1]];
        triangle[2]  =   triangle_resource.vertices[triangle_resource.indices[primitive_index * 3 + 2]];
        float3 normal = getNormal(triangle[0], triangle[1], triangle[2], bary);

        float4x4 object_to_world(1.0f);
        for (int column = 0; column < 4; column++)
            for (int row = 0; row < 3; row++)
                object_to_world[column][row] = instances[instance_index].transformationMatrix[column][row];
        
        normal = normal * (intersection.triangle_front_facing ? 1.f : -1.f);
        normal = normalize((object_to_world * float4(normal, 0.f)).xyz);
        float2 uv = getUV(triangle[0], triangle[1], triangle[2], bary);

        // Make SurfaceInteraction
        si.p = ray.origin + intersection.distance * ray.direction;
        si.n = normal;
        si.uv = uv;
        si.front_face = intersection.triangle_front_facing;
        si.mat = materials[triangle[0].material_id];
        si.textures = textures;
        si.w_o = -ray.direction;
        si.w_i = bsdf_sample(si, f_pdf, rng);

        if (f_pdf <= 0) break;
        f = bsdf_eval(si, si.w_i, si.w_o, rng);
        throughput = f * throughput / f_pdf;

        ray.origin = si.p;
        ray.direction = si.w_i;
        ray.min_distance = 0.0001f * uniforms.scene_scale;
        ray.max_distance = INFINITY;

        if (i > MIN_RR_DEPTH) {
            float q = max(throughput.x, max(throughput.y, throughput.z));

            if (next_randomf(rng) > q) {
                break;
            } else {
                throughput = throughput / (1.f - q);
            }
        }
    }

    return float4(L, 1);
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

[[intersection(triangle, triangle_data, instancing)]]
bool alphatest( 
                         uint                 primitive_index[[primitive_id]],
                         uint                 geometry_index[[geometry_id]],
                         float2               barycentric_coordinate[[barycentric_coord]],
                device   Resource            *resources[[buffer(0)]],
                constant OpenPBRMaterial     *materials[[buffer(1)]],
                device   Texture             *textures[[buffer(2)]]
            ) {

    constexpr sampler sam(min_filter::nearest, mag_filter::nearest, mip_filter::none);

    device Resource& triangle_resource = *(device Resource *)((device char *)resources + sizeof(Resource) * geometry_index);
    Vertex v0 = triangle_resource.vertices[triangle_resource.indices[primitive_index * 3 + 0]];
    Vertex v1 = triangle_resource.vertices[triangle_resource.indices[primitive_index * 3 + 1]];
    Vertex v2 = triangle_resource.vertices[triangle_resource.indices[primitive_index * 3 + 2]];
    if (materials[v0.material_id].base_color_texid < 0) return true;
    float3 bary = float3(1.f - barycentric_coordinate.x - barycentric_coordinate.y, barycentric_coordinate.x, barycentric_coordinate.y);
    float2 uv = getUV(v0, v1, v2, bary);

    float alpha = textures[materials[v0.material_id].base_color_texid].texture.sample(sam, uv).a;

    return alpha > 0.05f;
}

kernel void pathtracer(
            uint2                                                  tid                       [[thread_position_in_grid]],
            texture2d<float, access::read>                         src_tex                   [[texture(0)]],
            texture2d<float, access::write>                        dst_tex                   [[texture(1)]],
            instance_acceleration_structure                        acceleration_structure    [[buffer(0)]],
            intersection_function_table<triangle_data, instancing> intersection_function_table [[buffer(1)]],
            constant Uniforms&                                     uniforms                  [[buffer(2)]],
            constant MTLAccelerationStructureInstanceDescriptor   *instances                 [[buffer(3)]],
            device   Resource                                     *resources                 [[buffer(4)]],
            constant OpenPBRMaterial                              *materials                 [[buffer(5)]],
            device   Texture                                      *textures                  [[buffer(6)]]
            
        ) {

        if (tid.x > uniforms.viewport_size.x || tid.y > uniforms.viewport_size.y) return;

        uint pixel = tid.y * uniforms.viewport_size.x + tid.x;
        RNG rng = make_random(pixel, uniforms.frame_number);

        float2 uv = (float2)tid / uniforms.viewport_size;
        uv = uv - 0.5f;
        float2 d = uv + (next_random2f(rng) / uniforms.viewport_size);
        ray ray = spawnRay(d, uniforms);
        float4 L = traceRay(ray,
                            acceleration_structure,
                            intersection_function_table,
                            uniforms,
                            instances,
                            resources,
                            materials,
                            textures,
                            rng);

        L = clamp(L, 0.f, 10.f);
        if (uniforms.frame_number > 0) 
            L = ((uniforms.frame_number-1) * src_tex.read(tid) + L) / uniforms.frame_number;
        dst_tex.write(L, tid);
}
