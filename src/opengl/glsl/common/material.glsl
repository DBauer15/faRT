/*
 * Diffuse Reflection
 *
 */
vec3 sample_diffuse(SurfaceInteraction si, RNG rng) {
    return vec3(0);
}

float pdf_diffuse(SurfaceInteraction si, vec3 w) {
    return 0.f;
}

vec3 eval_diffuse(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    return si.mat.base_color * dot(w_i, si.n) * ONE_OVER_PI;
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

vec3 bsdf_eval(SurfaceInteraction si, vec3 w_i, vec3 w_o) {
    vec3 f = vec3(0.f);
    f += eval_diffuse(si, w_i, w_o);
    return f;
}

// TODO: Eventually replace this with a material model
//vec3 sampleBrdf(vec2 rand, vec3 n, inout vec3 w_i, inout float pdf) {
    //w_i = randomCosineHemispherePoint(rand, n);
    //pdf = dot(w_i, n) * ONE_OVER_PI;

    //vec3 c = vec3(0.85f);
    //float theta_i = dot(n, w_i);
    //return theta_i * c * ONE_OVER_PI;
//}
