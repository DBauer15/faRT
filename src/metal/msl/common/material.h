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
 * General Functions
 *
 */
float3 fresnelSchlick(float3 f0, float cos_theta) {
    return f0 + (float3(1.f) - f0) * pow(1.f - cos_theta, 5.f);
}

float ggxGeomtric(float roughness, float cos_theta_i, float cos_theta_o) {
    float k = roughness * roughness / 2.f;
    float g_i = cos_theta_i / max(FLT_MIN, (cos_theta_i*(1.f-k) + k));
    float g_o = cos_theta_o / max(FLT_MIN, (cos_theta_o*(1.f-k) + k));
    return g_o * g_i;
}

float ggxShadowing(float roughness, float cos_theta_h) {
    float cos_theta_h2 = cos_theta_h * cos_theta_h;
    float a2 = roughness * roughness;
    float d = (cos_theta_h2 * (a2 - 1.f)) + 1.f;
    return a2 / max(FLT_MIN, d * d) * ONE_OVER_PI;
}

float pdf_ggx(thread const SurfaceInteraction    &si, 
              float3                             w_i, 
              float3                             w_o) 
{
    float theta_i = dot(si.n, w_i);
    if (theta_i < 0.f) return 0.f;

    float3 h = normalize(w_i + w_o);
    float ndoth = dot(si.n, h);
    float G = ggxShadowing(si.mat.specular_roughness, ndoth);

    return G * ndoth / max(FLT_MIN, 4 * dot(w_o, h));
}

float pdf_ggx_transmission(thread const SurfaceInteraction  &si,
                           float3                           w_i,
                           float3                           w_o)
{
    float ldotn = dot(-si.n, w_i);
    if (ldotn < 0.f) return 0.f;
    float ior_from = 1.f;
    float ior_to = si.mat.specular_ior;
    if (!si.front_face) {
        ior_from = ior_to;
        ior_to = 1.f;
    }
    float3 h = normalize(-(ior_to * w_i + ior_from * w_o));
    float ndoth = abs(dot(si.n, h));
    float ldoth = abs(dot(w_i, h));
    float vdoth = abs(dot(w_o, h));
    float G = ggxShadowing(si.mat.specular_roughness, ndoth);

    return (ldoth * G) / (ldotn * ndoth);

}

float3 sample_ggx(thread const SurfaceInteraction    &si, 
                     thread RNG                         &rng) {
    float3 h = randomGGXMicrofacet(next_random2f(rng), si.n, si.mat.specular_roughness);
    float3 w = reflect(-si.w_o, h);

    return w;
}

float3 sample_ggx_transmission(thread const SurfaceInteraction   &si,
                              thread RNG                        &rng)
{
    float3 h = randomGGXMicrofacet(next_random2f(rng), si.n, si.mat.specular_roughness);
    
    float ior = si.mat.specular_ior;
    if (si.front_face)
        ior = 1.f / ior;
    float3 w = refract(-si.w_o, h, ior);

    return w;
}

float3 eval_ggx(thread const SurfaceInteraction   &si,
                float3                            w_i,
                float3                            w_o,
                float3                            w_h,
                float3                            f0)
{
    float hdotl = max(0.f, dot(w_i, w_h));
    float ndotv = max(FLT_MIN, dot(si.n, w_o));
    float ndotl = max(FLT_MIN, dot(si.n, w_i));
    float ndoth = max(0.f, dot(si.n, w_h));

    // fresnel
    float3 F = si.mat.specular_weight * si.mat.specular_color * fresnelSchlick(f0, hdotl);
    
    // geometric
    float D = ggxGeomtric(si.mat.specular_roughness, ndotl, ndotv);

    // shadowing
    float G = ggxShadowing(si.mat.specular_roughness, ndoth);

    return (F * G * D) / (4.f * ndotv /* * ndotl */) /* * ndotl */; // ndotl (theta_i) left out for numerical robustness
}

float3 eval_ggx_transmission(thread const SurfaceInteraction    &si,
                             float3                             w_i,
                             float3                             w_o,
                             float3                             w_h,
                             float3                             f0)
{
    float3 n = si.front_face ? si.n : -si.n;
    float hdotl = abs(dot(w_i, w_h));
    float hdotv = abs(dot(w_o, w_h));
    float ndotv = max(FLT_MIN, abs(dot(n, w_o)));
    float ndotl = max(FLT_MIN, abs(dot(n, w_i)));
    float ndoth = max(FLT_MIN, abs(dot(n, w_h)));

    float ior_from = 1.f;
    float ior_to = si.mat.specular_ior;
    if (!si.front_face) {
        ior_from = ior_to;
        ior_to = 1.f;
    }

    // fresnel
    float3 F = si.mat.specular_weight * si.mat.specular_color * fresnelSchlick(f0, hdotl);
    
    // geometric
    float D = ggxGeomtric(si.mat.specular_roughness, ndotl, ndotv);

    // shadowing
    float G = ggxShadowing(si.mat.specular_roughness, ndoth);

    float term0 = ((hdotl * hdotv) / (ndotl * ndotv)); 
    float3 term1 = (ior_from * ior_from) * (float(1.f) - F) * G * D;
    float term2 = pow(ior_to * dot(w_h, w_i) + ior_from * dot(w_h, w_o), 2);

    return term0 * term1 * ndotl;

    return ndotl * term0 * term1 / term2;
}

float pdf_lambert(thread const SurfaceInteraction   &si, 
                  float3                            w_i, 
                  float3                            w_o) 
{
    float theta_i = dot(si.n, w_i);
    if (theta_i < 0.f) return 0.f;
    return theta_i * ONE_OVER_PI;
}

float3 sample_lambert(thread const SurfaceInteraction   &si, 
                      thread RNG                        &rng) 
{
    float3 w = randomCosineHemispherePoint(next_random2f(rng), si.n);
    return w;
}

/* 
 * PBR Components
 *
 */
float3 eval_metal(thread const SurfaceInteraction   &si,
                  float3                            w_i,
                  float3                            w_o)
{
    constexpr sampler sam(address::repeat, filter::linear);

    // base reflectance
    float3 f0 = si.mat.base_color;
    if (si.mat.base_color_texid >= 0)
        f0 = si.textures[si.mat.base_color_texid].texture.sample(sam, si.uv).rgb;
    f0 = f0 * si.mat.base_weight;
    
    // half angle vector
    float3 h = normalize(w_i + w_o);
    return eval_ggx(si, w_i, w_o, h, f0);
}

float3 eval_glossy(thread const SurfaceInteraction  &si, 
                   float3                           w_i, 
                   float3                           w_o) 
{
    // base reflectance
    float3 f0 = float3(1.f) * pow((1.f - si.mat.specular_ior) / (1.f + si.mat.specular_ior), 2.f);
    
    // half angle vector
    float3 h = normalize(w_i + w_o);
    return eval_ggx(si, w_i, w_o, h, f0);
}

float3 eval_glossy_transmission(thread const SurfaceInteraction &si,
                                float3                          w_i,
                                float3                          w_o)
{
    if (dot(si.n, w_i) > 0.f) return float3(0.f);
    float ior_from = 1.f; // air
    float ior_to = si.mat.specular_ior; // medium
    if (!si.front_face) { // we are in the medium going out -> switch iors
        ior_from = ior_to;
        ior_to = 1.f;
    } 
    // float3 h = normalize(w_i + w_o * ior_from / ior_to);
    float3 h = normalize(-(ior_to * w_i + ior_from * w_o));

    // base reflectance
    float3 f0 = float3(1.f) * pow((ior_from - ior_to) / (ior_from + ior_to), 2.f);

    return si.mat.specular_color * eval_ggx_transmission(si, w_i, w_o, h, f0);
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

/*
 * Top-Level BxDF functions called by the renderer
 *
 */
float bsdf_pdf(const thread SurfaceInteraction    &si, 
               float3                             w_i, 
               float3                             w_o) 
{
    float n_components = si.mat.transmission_weight > 0.f ? 3.f : 2.f;
    float diffuse = pdf_lambert(si, w_i, w_o);
    float glossy = pdf_ggx(si, w_i, w_o);
    float transmission = si.mat.transmission_weight > 0.f ? pdf_ggx_transmission(si, w_i, w_o) : 0.f;

    return (diffuse + glossy + transmission) / n_components;
}

float3 bsdf_sample(thread const SurfaceInteraction  &si, 
                   thread float                     &pdf, 
                   thread RNG                       &rng)
{
    float3 w;

    // TODO: Find better heuristic to choose the sampler
    float n_components = si.mat.transmission_weight > 0.f ? 3.f : 2.f;
    int bsdf_component = next_randomf(rng) * n_components;
    // bsdf_component = 2;
    if (bsdf_component == 0) {
        w = sample_lambert(si, rng);
    } else if (bsdf_component == 1) {
        w = sample_ggx(si, rng);
    } else {
        w = sample_ggx_transmission(si, rng);
    }

    // pdf = pdf_ggx_transmission(si, w, si.w_o);
    pdf = bsdf_pdf(si, w, si.w_o);
    return w;
}

float3 bsdf_eval(thread const SurfaceInteraction    &si, 
                 float3                             w_i, 
                 float3                             w_o, 
                 thread RNG                         &rng)
{
    float3 glossy_transmission = si.mat.transmission_weight > 0.f ? eval_glossy_transmission(si, w_i, w_o) : 0.f;
    // return glossy_transmission;
    float3 glossy = si.mat.specular_weight > 0.f ? eval_glossy(si, w_i, w_o) : 0.f;
    float3 E_glossy = si.mat.specular_weight > 0.f ? ggx_reflectance(si, w_o, rng) : 0.f;
    float3 metal = si.mat.base_metalness > 0.f ? eval_metal(si, w_i, w_o) : 0.f;

    float3 diffuse = eval_diffuse(si, w_i, w_o);

    float3 t_dielectric = mix(diffuse, glossy_transmission, si.mat.transmission_weight);
    float3 dielectric = glossy + (float3(1.f) - E_glossy) * t_dielectric;

    return mix(dielectric, metal, si.mat.base_metalness);
}
