/*
 * Implementation of the OpenPBR material model
 * References:
 * https://academysoftwarefoundation.github.io/OpenPBR/#model/
 * https://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
 *
 */

#include <metal_stdlib>
#include "random.h"
#include "sampling.h"

using namespace metal;

/*
 * Glossy Diffuse Reflection
 *
 */

/* GGX Microfacet */
float pdf_glossy(thread const SurfaceInteraction    &si, 
                 float3                             w_i, 
                 float3                             w_o) 
{
    float theta_i = dot(si.n, w_i);
    if (theta_i < 0.f) return 0.f;

    float3 h = normalize(w_i + w_o);
    float theta_h = dot(si.n, h);
    float theta_h_2 = theta_h * theta_h;
    float a2 = si.mat.specular_roughness * si.mat.specular_roughness;
    float d = (theta_h_2 * (a2 - 1.f)) + 1.f;
    float D = a2 / max(FLT_MIN, d * d) * ONE_OVER_PI;

    return D * theta_h / max(FLT_MIN, 4 * dot(w_o, h));
}

float3 sample_glossy(thread const SurfaceInteraction    &si, 
                     thread RNG                         &rng) {
    float3 h = randomGGXMicrofacet(next_random2f(rng), si.n, si.mat.specular_roughness);
    float3 w = reflect(-si.w_o, h);

    return w;
}

float3 eval_glossy(thread const SurfaceInteraction  &si, 
                   float3                           w_i, 
                   float3                           w_o) 
{
    float3 h = normalize(w_i + w_o);

    float3 f0 = float3(1.f) * pow((1.f - si.mat.specular_ior) / (1.f + si.mat.specular_ior), 2.f);
    float3 F = f0 + (float3(1.f) - f0) * pow(1.f - max(0.f, dot(w_i, h)), 5.f);

    float k = si.mat.specular_roughness * si.mat.specular_roughness / 2.f;
    float ndotv = max(FLT_MIN, dot(si.n, w_o));
    float ndotl = max(FLT_MIN, dot(si.n, w_i));
    float g_v = ndotv / max(FLT_MIN, (ndotv*(1.f-k) + k));
    float g_l = ndotl / max(FLT_MIN, (ndotl*(1.f-k) + k));
    float G = g_v * g_l;

    float ndoth = max(0.f, dot(h, si.n));
    float ndoth2 = ndoth * ndoth;
    float a2 = si.mat.specular_roughness * si.mat.specular_roughness;
    float d = (ndoth2 * (a2 - 1.f)) + 1.f;
    float D = a2 / max(FLT_MIN, d * d * PI);

    return float3(1.f) * (F * G * D) / (4.f * ndotv /* * ndotl */) /* * ndotl */; // ndotl (theta_i) left out for numerical robustness
}

/* Estimator for GGX Microfacet reflectanace */
float3 ggx_reflectance(thread const SurfaceInteraction  &si, 
                       float3                           w_o, 
                       thread RNG                       &rng) {
    float3 reflectance = float3(0.f);
    float3 w_i;
    const uint samples = 8;
    for (uint i = 0; i < samples; i++) {
        w_i = randomHemispherePoint(next_random2f(rng), si.n);
        reflectance += eval_glossy(si, w_i, w_o);
    }
    return reflectance / samples;
}

/* Lambertian Diffuse */
float pdf_diffuse(thread const SurfaceInteraction   &si, 
                  float3                            w_i, 
                  float3                            w_o) 
{
    float theta_i = dot(si.n, w_i);
    if (theta_i < 0.f) return 0.f;
    return theta_i * ONE_OVER_PI;
}

float3 sample_diffuse(thread const SurfaceInteraction   &si, 
                      thread RNG                        &rng) 
{
    float3 w = randomCosineHemispherePoint(next_random2f(rng), si.n);
    return w;
}

float3 eval_diffuse(thread const SurfaceInteraction     &si, 
                    float3                              w_i, 
                    float3                              w_o) 
{
    constexpr sampler sam(address::repeat, filter::linear);

    float3 f = si.mat.base_color;
    if (si.mat.base_color_texid >= 0)
        f = si.textures[si.mat.base_color_texid].texture.sample(sam, si.uv).rgb;
    f *= si.mat.base_weight * dot(w_i, si.n) * ONE_OVER_PI;
    return f;
}

/* Compound PBR Model */
float3 eval_glossy_diffuse(thread const SurfaceInteraction    &si, 
                           float3                             w_i, 
                           float3                             w_o, 
                           thread RNG                         &rng) 
{
    // Compute reflectance estimate for albedo scaling
    float3 E_glossy = ggx_reflectance(si, w_o, rng);
    return (1.f - E_glossy) * eval_diffuse(si, w_i, w_o) + eval_glossy(si, w_i, w_o);
}


/*
 * Top-Level BxDF functions called by the renderer
 *
 */
float bsdf_pdf(const thread SurfaceInteraction    &si, 
               float3                             w_i, 
               float3                             w_o) 
{
    float diffuse = pdf_diffuse(si, w_i, w_o);
    float glossy = pdf_glossy(si, w_i, w_o);

    return (diffuse + glossy) / 2.f;
}

float3 bsdf_sample(thread const SurfaceInteraction  &si, 
                   thread float                     &pdf, 
                   thread RNG                       &rng)
{
    float3 w;

    // TODO: Find better heuristic to choose the sampler
    float q = next_randomf(rng);
    if (q < 0.5f) {
        w = sample_diffuse(si, rng);
    } else {
        w = sample_glossy(si, rng);
    }

    pdf = bsdf_pdf(si, w, si.w_o);
    return w;
}

float3 bsdf_eval(thread const SurfaceInteraction    &si, 
                 float3                             w_i, 
                 float3                             w_o, 
                 thread RNG                         &rng)
{
    const float rec_n_components = 1.f;
    float3 f = float3(0.f);
    f += eval_glossy_diffuse(si, w_i, w_o, rng);
    return f * rec_n_components;
}
