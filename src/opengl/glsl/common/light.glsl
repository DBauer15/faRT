/*
 * Implementation of a lighting model
 * References:
 * https://pbr-book.org/4ed/Light_Sources
 *
 */

#define DISTANT_LIGHT   0
#define INFINITE_LIGHT  1
#define POINT_LIGHT     2
#define SPHERE_LIGHT    3
#define DISK_LIGHT      4

vec3 infinite_Le(const Light light,
                 vec3 w_i,
                 inout RNG rng) 
{
    vec3 jitter = next_random3f(rng) * 2.f - 1.f;
    //w_i = normalize(w_i + jitter * 0.015f);
    vec3 L = light.L;
    if (light.map_texid >= 0) {
        float y = w_i.y * 0.5f + 0.5f;
        float theta = 0.f;
        theta = atan(w_i.z, w_i.x) * ONE_OVER_TWO_PI + 0.5f;

        L *= texture(textures[light.map_texid], vec2(theta, y)).rgb;

    }
    return L;
}

vec3 sphere_Li(const Light light,
               const SurfaceInteraction si,
               inout RNG rng)
{
    Ray ray;
    ray.o = si.p;
    ray.d = si.w_i;
    ray.t = 1e30f;
    if (intersectSphere(ray, light.from, light.radius)) {
        return light.L;
    }

    return vec3(0.f);
}

/*
 * Top-Level light functions called by the renderer
 *
 */
vec3 direct_light(const SurfaceInteraction si,
                  inout RNG                rng)
{
    if (lights.length() == 0) return vec3(0.f);
    int light_id = int(next_randomf(rng) * lights.length());
    float light_selection_pdf = 1.f / lights.length();

    vec3 L = vec3(0.f);

    Light light = lights[light_id];
    if (light.type == INFINITE_LIGHT)
        L = infinite_Le(light, si.w_i, rng);
    if (light.type == SPHERE_LIGHT)
        L = sphere_Li(light, si, rng);

    return L;
    return L / light_selection_pdf;
}
