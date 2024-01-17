#define PI 3.14159265358979323846
#define ONE_OVER_PI 0.31830988618379067154
#define ONE_OVER_TWO_PI 0.15915494309189533577
#define ONE_OVER_FOUR_PI 0.07957747154594766788
#define PI_OVER_TWO 1.57079632679489661923
#define PI_OVER_FOUR 0.78539816339744830961

/*
 * From: Hash Functions for GPU Rendering
 * Reference: 
 * https://jcgt.org/published/0009/03/02/paper.pdf
 */
uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

/* 
 * Generates a random vec3 in [0, 1)
 * Reference: 
 * https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
 */
vec3 random3(vec3 f) {
    return uintBitsToFloat((pcg3d(floatBitsToUint(f)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
}

float random(vec2 xy, float z) {
    return random3(vec3(xy, z)).x;
}

/* 
 * Reorients a vector around a normal
 *
 */
vec3 reorient(vec3 dir, vec3 normal) {
    vec3 tangent = cross(dir, normal);
    vec3 bitangent = cross(tangent, normal);
    tangent = cross(bitangent, normal);

    // Orient towards normal
    mat3 tbn = mat3(tangent, bitangent, normal);
    return tbn * dir;
}

/**
 * Generate a uniformly distributed random point on the unit disk 
 * 
 * Reference:
 * https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
 */
vec2 randomDiskPoint(vec3 rand) {
    vec2 u_offset = 2.f * rand.xy - vec2(1.f);
    if (u_offset.x == 0.f && u_offset.y == 0.f)
        return vec2(0.f);
    
    float theta, r;
    if (abs(u_offset.x) > abs(u_offset.y)) {
        r = u_offset.x;
        theta = PI_OVER_FOUR * (u_offset.y / u_offset.x);
    } else { 
        r = u_offset.y;
        theta = PI_OVER_TWO - PI_OVER_FOUR * (u_offset.x / u_offset.y);
    }
    return r * vec2(cos(theta), sin(theta));
}

/**
 * Generate a uniformly distributed random point on the unit-sphere.
 * 
 * Reference:
 * http://mathworld.wolfram.com/SpherePointPicking.html
 */
vec3 randomSpherePoint(vec3 rand) {
    float phi = rand.x * 2.f * PI;
    float cosTheta = 1.f - rand.y * 2.f;
    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta = sqrt(max(0, 1.f - cosTheta2));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;
    return vec3(x, y, z);
}

/**
* Generate a uniformly distributed random point on the unit-hemisphere
* Reference:
* https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
*
*/
vec3 randomHemispherePoint(vec3 rand, vec3 n) {
    float z = rand.x;
    float r = sqrt(max(0.f, 1.f - z * z));
    float phi = 2.f * PI * rand.y;

    vec3 dir = vec3(r * cos(phi), r * sin(phi), z);
    return reorient(dir, n);
}

/**
* Generate a cosine weighted random point on the unit-hemisphere
* Reference:
* https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
*
*/ 
vec3 randomCosineHemispherePoint(vec3 rand, vec3 n) {
    vec2 p = randomDiskPoint(rand);
    float z = sqrt(max(0.f, 1.f - p.x*p.x - p.y*p.y));
    vec3 dir = vec3(p, z);

    return reorient(dir, n);
}

// TODO: Eventually replace this with a material model
vec3 sampleBrdf(vec3 rand, vec3 n, inout vec3 w_i, inout float pdf) {
    w_i = randomCosineHemispherePoint(rand, n);
    pdf = dot(w_i, n) * ONE_OVER_PI;

    vec3 c = vec3(0.2f);
    float theta_i = dot(n, w_i);
    return theta_i * c * ONE_OVER_PI;
}
