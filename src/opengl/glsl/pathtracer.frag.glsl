#version 450
#extension GL_GOOGLE_include_directive : enable 

#define MIN_RR_DEPTH 3
#define MAX_BOUNCES 5

#include "common/color.glsl"
#include "common/types.glsl"
#include "common/data.glsl"
#include "common/random.glsl"
#include "common/sampling.glsl"
#include "common/material.glsl"
#include "common/intersect.glsl"

uniform sampler2D u_frag_color_accum;
out vec4 frag_color;

vec4 miss(Ray ray) {
    vec4 sky = vec4(70./255., 169./255., 235./255., 1.f);
    vec4 haze = vec4(127./255., 108./255., 94./255., 1.f);
    vec4 background = mix(sky, haze, (ray.d.y + 1.f) /2.f);
    return background;
}

vec4 closestHit(SurfaceInteraction si, inout RNG rng) {

    vec3 L = vec3(0.f);
    vec3 throughput = vec3(1.f);
    vec2 uv = gl_FragCoord.xy / u_viewport_size;

    vec3 f;
    float f_pdf;
    for (int i = 0; i < MAX_BOUNCES; i++) {
        si.w_i = bsdf_sample(si, f_pdf, rng);
        if (f_pdf <= 0.f) break;
        f = bsdf_eval(si, si.w_i, si.w_o, rng);
        throughput = f * throughput / f_pdf;

        Ray ray;
        ray.o = si.p + 0.00001f * u_scene_scale * si.n;
        ray.d = si.w_i;
        ray.rD = 1.f / si.w_i;
        ray.t = 1e30f;

        si = intersect(ray);

        // Ray left the scene, apply miss shader
        if (!si.valid) {
            L = throughput * miss(ray).rgb;
            break;
        }

        // Russian roulette termination
        if (i > MIN_RR_DEPTH) {
            float q = max(throughput.x, max(throughput.y, throughput.z));

            if (next_randomf(rng) > q) {
                break;
            } else {
                throughput = throughput / (1 - q);
            }
        }
    }

    return vec4(L, 1.f);
}

Ray spawnRay(vec2 d) {
    Ray ray;
    ray.o = u_camera.eye;
    ray.d = normalize(u_camera.dir + 
                        u_aspect_ratio * (d.x-.5f) * cross(u_camera.dir, u_camera.up) +
                        (d.y-.5f) * u_camera.up);
    ray.rD = 1.f / ray.d;
    ray.t = 1e30f;
    
    return ray;
}

void main() {
    uint pixel_id = uint(gl_FragCoord.y * u_viewport_size.x + gl_FragCoord.x);
    RNG rng = make_random(pixel_id, u_frame_no);

    vec4 L = vec4(0.f);
    vec2 uv = vec2(gl_FragCoord.xy) / u_viewport_size;
    vec2 d = uv + (next_random2f(rng) / u_viewport_size);
    Ray ray = spawnRay(d);

    SurfaceInteraction si = intersect(ray);
    if (si.valid)
        L = closestHit(si, rng);
    else
        L = miss(ray);

    L = clamp(L, 0.f, 10.f);
    frag_color = (u_frame_no * texture(u_frag_color_accum, uv) + L) / (u_frame_no + 1.f);
}
