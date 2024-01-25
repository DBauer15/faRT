/*
 * Implementation of the OpenPBR material model
 * References:
 * https://academysoftwarefoundation.github.io/OpenPBR/#model/
 * https://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
 *
 */

/*
 * Glossy Diffuse Reflection
 *
 */

/* GGX Microfacet */
float pdf_glossy(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    float theta_i = dot(si.n, w_i);
    float theta_o = dot(si.n, w_o);
    if (theta_i < 0.f || theta_o < 0.f) return 0.f;

    vec3 h = normalize(w_i + w_o);
    float theta_h = dot(si.n, h);
    float theta_h_2 = theta_h * theta_h;
    float a2 = si.mat.specular_roughness * si.mat.specular_roughness;
    float d = (theta_h_2 * (a2 - 1.f)) + 1.f;
    float D = a2 / max(FLT_MIN, d * d) * ONE_OVER_PI;

    return D * theta_h / max(FLT_MIN, 4 * dot(w_o, h));
}

vec3 sample_glossy(SurfaceInteraction si, RNG rng) {
    vec3 h = randomGGXMicrofacet(next_random2f(rng), si.n, si.mat.specular_roughness);
    vec3 w = normalize(2.f * dot(si.w_o, h) * h - si.w_o);

    return w;
}

vec3 eval_glossy(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    vec3 h = normalize(w_i + w_o);

    vec3 f0 = vec3(1.f) * pow((1 - si.mat.specular_ior) / (1 + si.mat.specular_ior), 2.f);
    vec3 F = f0 + (vec3(1.f) - f0) * pow(1.f - dot(w_o, h), 5.f);

    float k = si.mat.specular_roughness * si.mat.specular_roughness / 2.f;
    float ndotv = dot(si.n, w_o);
    float ndotl = dot(si.n, w_i);
    float g_v = ndotv / max(FLT_MIN, (ndotv*(1.f-k) + k));
    float g_l = ndotl / max(FLT_MIN, (ndotl*(1.f-k) + k));
    float G = g_v * g_l;

    float ndoth = max(0, dot(h, si.n));
    float ndoth2 = ndoth * ndoth;
    float a2 = si.mat.specular_roughness * si.mat.specular_roughness;
    float d = (ndoth2 * (a2 - 1.f)) + 1.f;
    float D = a2 / max(FLT_MIN, d * d * PI);

    return vec3(1.f) * (F * G * D) / (4.f * ndotv /* * ndotl */) /* * ndotl */; // ndotl (theta_i) left out for numerical robustness
}

/* Estimator for GGX Microfacet reflectanace */
vec3 ggx_reflectance(SurfaceInteraction si, vec3 w_o, RNG rng) {
    vec3 reflectance = vec3(0.f);
    vec3 w_i;
    const uint samples = 8;
    for (int i = 0; i < samples; i++) {
        w_i = randomHemispherePoint(next_random2f(rng), si.n);
        reflectance += eval_glossy(si, w_i, w_o);
    }
    return reflectance / samples;
}

/* Lambertian Diffuse */
float pdf_diffuse(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    float theta_i = dot(si.n, w_i);
    float theta_o = dot(si.n, w_o);
    if (theta_i < 0.f || theta_o < 0.f) return 0.f;
    return theta_i * ONE_OVER_PI;
}

vec3 sample_diffuse(SurfaceInteraction si, RNG rng) {
    vec3 w = randomCosineHemispherePoint(next_random2f(rng), si.n);
    return w;
}

vec3 eval_diffuse(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    return si.mat.base_weight * si.mat.base_color * dot(w_i, si.n) * ONE_OVER_PI;
}

vec3 eval_glossy_diffuse(SurfaceInteraction si, vec3 w_i, vec3 w_o, RNG rng) {
    // Compute reflectance estimate for albedo scaling
    //return eval_diffuse(si, w_i, w_o);
    //vec3 E_glossy = ggx_reflectance(si, w_o, rng);
    return eval_diffuse(si, w_i, w_o) + eval_glossy(si, w_i, w_o);
    //return (1.f - E_glossy) * eval_diffuse(si, w_i, w_o) + eval_glossy(si, w_i, w_o);
}


/*
 * Top-Level BxDF functions called by the renderer
 *
 */
float bsdf_pdf(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    float diffuse = pdf_diffuse(si, w_i, w_o);
    float glossy = pdf_glossy(si, w_i, w_o);

    return (diffuse + glossy) / 2.f;
}

vec3 bsdf_sample(SurfaceInteraction si, inout float pdf, RNG rng) {
    vec3 w;

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

vec3 bsdf_eval(SurfaceInteraction si, vec3 w_i, vec3 w_o, RNG rng) {
    const float rec_n_components = 1.f;
    vec3 f = vec3(0.f);
    f += eval_glossy_diffuse(si, w_i, w_o, rng);
    return f * rec_n_components * dot(w_i, si.n);
}
