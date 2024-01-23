/*
 * Glossy Diffuse Reflection
 *
 */

/* GGX Microfacet */
vec3 eval_glossy(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    //vec3 h = normalize((w_i + w_o) / abs(w_i + w_o));
    vec3 h = normalize(w_i + w_o);

    float ior = 1.5f;
    vec3 f0 = vec3(1.f) * pow((1 - ior) / (1 + ior), 2);
    vec3 F = f0 + (vec3(1.f) - f0) * pow(1.f - dot(w_o, h), 5.f);

    float k = si.mat.base_roughness * si.mat.base_roughness / 2;
    float ndotv = dot(si.n, w_o);
    float ndotl = dot(si.n, w_i);
    float g_v = ndotv / (ndotv*(1-k) + k);
    float g_l = ndotl / (ndotl*(1-k) + k);
    float G = g_v * g_l;

    float ndoth = dot(h, si.n);
    float a2 = si.mat.base_roughness * si.mat.base_roughness;
    float d = ((ndoth * a2 - ndoth) * ndoth + 1);
    float D = a2 / max(0.0000001f, d * d * PI);

    return vec3(1.f) * (F * G * D) / (4 * ndotv * ndotl);
}

/* Estimator for GGX Microfacet reflectanace */
vec3 ggx_reflectance(SurfaceInteraction si, vec3 w_o, RNG rng) {
    vec3 reflectance = vec3(0.f);
    vec3 w_i;
    const uint samples = 4;
    for (int i = 0; i < samples; i++) {
        w_i = randomHemispherePoint(next_random2f(rng), si.n);
        reflectance += eval_glossy(si, w_i, w_o);
    }
    return reflectance / samples;
}

/* Lambertian Diffuse */
vec3 eval_diffuse(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    return si.mat.base_weight * si.mat.base_color * dot(w_i, si.n) * ONE_OVER_PI;
}

vec3 eval_glossy_diffuse(SurfaceInteraction si, vec3 w_i, vec3 w_o, RNG rng) {
    // Compute reflectance estimate for albedo scaling
    //return eval_glossy(si, w_i, w_o);
    vec3 E_glossy = ggx_reflectance(si, w_o, rng);
    return (1.f - E_glossy) * eval_diffuse(si, w_i, w_o) + eval_glossy(si, w_i, w_o);
}


/*
 * Top-Level BxDF functions called by the renderer
 *
 */
vec3 bsdf_sample(SurfaceInteraction si, RNG rng) {
    vec3 w = randomCosineHemispherePoint(next_random2f(rng), si.n);
    return w;
}

float bsdf_pdf(SurfaceInteraction si, vec3 w) {
    float theta = dot(w, si.n);
    if (theta < 0.f)
        return 0.f;
    return theta * ONE_OVER_PI;
}

vec3 bsdf_eval(SurfaceInteraction si, vec3 w_i, vec3 w_o, RNG rng) {
    const float rec_n_components = 1.f;
    vec3 f = vec3(0.f);
    f += eval_glossy_diffuse(si, w_i, w_o, rng);
    return f * rec_n_components * dot(w_i, si.n);
}
