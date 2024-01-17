#version 450
#extension GL_GOOGLE_include_directive : enable 

#define MIN_RR_DEPTH 3

#include "common/types.glsl"
#include "common/data.glsl"
#include "common/random.glsl"
#include "common/intersect.glsl"

uniform sampler2D u_frag_color_accum;
out vec4 frag_color;

vec4 miss(Ray ray) {
    float strength = 3.5f;
    vec4 sky = vec4(70./255., 169./255., 235./255., 1.f);
    vec4 haze = vec4(127./255., 108./255., 94./255., 1.f);
    vec4 background = mix(haze, sky, (ray.d.y + 1.f) /2.f);
    return background * strength;
}

vec4 closestHit(SurfaceInteraction si) {
    vec3 L = vec3(0.f);
    vec3 throughput = vec3(1.f);
    vec2 uv = gl_FragCoord.xy / u_viewport_size;

    vec3 f;
    float f_pdf;
    for (int i = 0; i < 5; i++) {
        f = sampleBrdf(random3(vec3(uv, u_frame_no+i)), si.n, si.w_i, f_pdf);
        throughput = f * throughput / f_pdf;

        Ray ray;
        ray.o = si.p + 0.001f * si.n;
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

            if (random(uv+i, u_frame_no+i) > q) {
                break;
            } else {
                throughput = throughput / (1 - q);
            }
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

    frag_color = (u_frame_no * texture(u_frag_color_accum, uv) + L) / (u_frame_no + 1.f);
}
