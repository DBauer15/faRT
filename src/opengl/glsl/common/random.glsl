#define PI 3.14159265359

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

/**
 * Generate a uniformly distributed random point on the unit disk towards the normal
 * 
 * Reference:
 * http://mathworld.wolfram.com/DiskPointPicking.html
 */
vec3 randomDiskPoint(vec3 rand, vec3 n) {
    float r = rand.x;
    float angle = rand.y * 2.0 * PI;
    float sr = sqrt(r);
    vec2 p = vec2(sr * cos(angle), sr * sin(angle));
    vec3 tangent = normalize(rand);
    vec3 bitangent = cross(tangent, n);
    tangent = cross(bitangent, n);

    // Orient towards normal
    return tangent * p.x + bitangent * p.y;
}

/**
 * Generate a uniformly distributed random point on the unit-sphere.
 * 
 * Reference:
 * http://mathworld.wolfram.com/SpherePointPicking.html
 */
vec3 randomSpherePoint(vec3 rand) {
    float phi = rand.x * 2.0 * PI;
    float cosTheta = rand.y * 2.0 - 1.0;
    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta = sqrt(1.0 - cosTheta2);
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;
    return vec3(x, y, z);
}

/**
* Generate a uniformly distributed random point on the unit-hemisphere
*
*/
vec3 randomHemispherePoint(vec3 rand, vec3 n) {
    vec3 v = randomSpherePoint(rand);
    return v * sign(dot(v, n));
}
