#define EPS 0.00000001
#define PI 3.14159265358979323846
#define ONE_OVER_PI 0.31830988618379067154
#define ONE_OVER_TWO_PI 0.15915494309189533577
#define ONE_OVER_FOUR_PI 0.07957747154594766788
#define PI_OVER_TWO 1.57079632679489661923
#define PI_OVER_FOUR 0.78539816339744830961
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38

/* 
 * Reorients a vector around a normal
 * References:
 * https://www.tandfonline.com/doi/abs/10.1080/2151237X.2009.10129274
 * https://graphics.pixar.com/library/OrthonormalB/paper.pdf
 *
 */
vec3 reorient(vec3 dir, vec3 normal) {
    //vec3 a = abs(normal);
    //uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    //uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    //uint zm = 1 ^ (xm | ym);

    //vec3 tangent = cross(vec3(xm, ym, zm), normal);
    //vec3 bitangent = cross(tangent, normal);

    //vec3 y = vec3(0, 1, 0);
    //if (dot(normal, y) < EPS)
        //y = vec3(0, 0, 1);
    //vec3 tangent = cross(normal, y);
    //vec3 bitangent = cross(normal, tangent);


    //TODO: Somehow sign(normal.z) causes issues; Keep this in mind if problems arise later for sphere sampling
    float sign = normal.z < 0.f ? -1.f : 1.f; //sign(normal.z);
    float a = -1.f / (sign + normal.z);
    float b = normal.x * normal.y * a;
    vec3 tangent = vec3(1.f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
    vec3 bitangent = vec3(b, sign + normal.y * normal.y * a, -normal.y);

    // Orient towards normal
    return dir.x * tangent + dir.y * bitangent + dir.z * normal;
}

/**
 * Generate a uniformly distributed random point on the unit disk 
 * 
 * Reference:
 * https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
 */
vec2 randomDiskPoint(vec2 rand) {
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
* Generate a uniformly distributed random point on the unit-hemisphere
* Reference:
* https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
*
*/
vec3 randomHemispherePoint(vec2 rand, vec3 n) {
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
vec3 randomCosineHemispherePoint(vec2 rand, vec3 n) {
    vec2 p = randomDiskPoint(rand);
    float z = sqrt(max(0.f, 1.f - p.x*p.x - p.y*p.y));
    vec3 dir = vec3(p, z);

    return reorient(dir, n);
}

/**
* Generates a random sample from the GGX NDF to get a microfacet
* Reference:
* https://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
*
*/ 
vec3 randomGGXMicrofacet(vec2 rand, vec3 n, float roughness) {
	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cos_theta_h = sqrt(max(0.f, (1.f-rand.x)/((a2-1.f)*rand.x+1.f) ));
	float sin_theta_h = sqrt(max(0.f, 1.f - cos_theta_h * cos_theta_h));
	float phi_h = rand.y * PI * 2.f;

    vec3 dir = vec3(sin_theta_h * cos(phi_h), sin_theta_h * sin(phi_h), cos_theta_h);
    return reorient(dir, n);
}
