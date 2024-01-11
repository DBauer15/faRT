#version 450
#extension GL_GOOGLE_include_directive : enable 

#include "common/types.glsl"
#include "common/data.glsl"
#include "common/random.glsl"
#include "common/intersect.glsl"
#include "common/color.glsl"

uniform sampler2D u_frag_color_accum;
out vec4 frag_color;

float light_probe(Ray ray) {
    SurfaceInteraction si = intersect(ray);
    if (si.valid)
        return 0.0f;
    else 
        return 1.f;
}

vec4 sample_env(Ray ray) {
    float strength = 3.5f;
    vec4 sky = vec4(70./255., 169./255., 235./255., 1.f);
    vec4 haze = vec4(255./255., 216./255., 189./255., 1.f);
    vec4 background = mix(haze, sky, (ray.d.y + 1.f) /2.f);
    return background * strength;
}

vec4 miss(Ray ray) {
    return sample_env(ray);
}

vec4 closestHit_ao(SurfaceInteraction si) {
    // Shade basic AO
    vec4 ao = vec4(0.f);
    vec2 uv = gl_FragCoord.xy / u_viewport_size;
    Ray ao_ray;
    ao_ray.o = si.p + 0.001f * si.n;
    ao_ray.t = 1e30f;
    ao_ray.d = randomHemispherePoint(random3(vec3(uv, u_frame_no)), si.n);
    ao_ray.rD = 1.f / ao_ray.d;
    ao = sample_env(ao_ray) * light_probe(ao_ray);

    return ao;
}

vec4 closestHit(SurfaceInteraction si) {
    
    vec3 L = vec3(0.f);
    vec3 throughput = vec3(1.f);
    vec2 uv = gl_FragCoord.xy / u_viewport_size;

    for (int i = 0; i < 8; i++) {
        si.w_i = randomHemispherePoint(random3(vec3(uv, u_frame_no+i)), si.n);
        throughput = throughput * abs(dot(si.w_i, si.n));

        Ray ray;
        ray.o = si.p + 0.001f * si.n;
        ray.d = si.w_i;
        ray.rD = 1.f / si.w_i;
        ray.t = 1e30f;

        si = intersect(ray);
        if (!si.valid) {
            L = throughput * sample_env(ray).rgb;
            break;
        }
    }

    return vec4(L, 1.f);
}

Ray spawnRay(vec2 uv) {
    Ray ray;
    ray.o = u_camera.eye;
    ray.d = normalize(u_camera.dir + 
                        u_aspect_ratio * (uv.x-.5f) * cross(u_camera.dir, u_camera.up) +
                        (uv.y-.5f) * u_camera.up);
    ray.rD = 1.f / ray.d;
    ray.t = 1e30f;
    
    return ray;
}

void main() {
    vec4 L = vec4(0.f);
    vec2 uv = gl_FragCoord.xy / u_viewport_size;
    Ray ray = spawnRay(uv);

    SurfaceInteraction si = intersect(ray);
    if (si.valid)
        L = closestHit(si);
    else
        L = miss(ray);

    L = gamma(tonemap_Exposure(L, 1.f));
    frag_color = (u_frame_no * texture(u_frag_color_accum, uv) + L) / (u_frame_no + 1.f);
}
