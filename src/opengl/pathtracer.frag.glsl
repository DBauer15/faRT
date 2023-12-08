#version 450

struct BVHNode {
    vec4 aabb_min;
    vec4 aabb_max;
    uint left_child;
    uint first_tri_index_id;
    uint tri_count;
    uint filler;
};

struct SurfaceInteraction {
    vec3 p;
    vec3 w_i;
    vec3 w_o;
};

struct Camera {
    vec3 eye;
    vec3 dir;
    vec3 up;
};

struct Ray {
    vec3 o;
    vec3 d;
    float t;
};

uniform uvec2 u_viewport_size;
uniform float u_aspect_ratio;
uniform Camera u_camera;

out vec4 frag_color;

layout(std430, binding = 0) buffer geometry0 {
    vec4 vertices [];
};

layout(std430, binding = 1) buffer geometry1 {
    uint indices [];
};

layout(std430, binding = 2) buffer accel0 {
    BVHNode bvh [];
};

bool intersectTriangle(inout Ray ray, int first_index) {
    vec3 v0 = vertices[indices[first_index+0]].xyz;
    vec3 v1 = vertices[indices[first_index+1]].xyz;
    vec3 v2 = vertices[indices[first_index+2]].xyz;

    const vec3 edge1 = v1 - v0;
    const vec3 edge2 = v2 - v0;
    const vec3 h = cross( ray.d, edge2 );
    const float a = dot( edge1, h );
    if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle
    const float f = 1 / a;
    const vec3 s = ray.o - v0;
    const float u = f * dot( s, h );
    if (u < 0 || u > 1) return false;
    const vec3 q = cross( s, edge1 );
    const float v = f * dot( ray.d, q );
    if (v < 0 || u + v > 1) return false;
    const float t = f * dot( edge2, q );
    if (t > 0.0001f) {
        ray.t = min( ray.t, t );
        return true;
    }
    return false;
}

bool intersectAABB(Ray ray, BVHNode node) {
    vec3 bmin = node.aabb_min.xyz;
    vec3 bmax = node.aabb_max.xyz;

    float tx1 = (bmin.x - ray.o.x) / ray.d.x, tx2 = (bmax.x - ray.o.x) / ray.d.x;
    float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
    float ty1 = (bmin.y - ray.o.y) / ray.d.y, ty2 = (bmax.y - ray.o.y) / ray.d.y;
    tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
    float tz1 = (bmin.z - ray.o.z) / ray.d.z, tz2 = (bmax.z - ray.o.z) / ray.d.z;
    tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
    return tmax >= tmin && tmin < ray.t && tmax > 0;
}

int traverseBVH(inout Ray ray) {
    int depth = 0;
    uint stack[32];
    int current = 0;
    stack[current] = 0;

    do {
        BVHNode node = bvh[stack[current--]];

        if (intersectAABB(ray, node)) {
            // Intersections, push left and right child on the stack for processing
            if (node.left_child > 0) { // 0 indicates leaf node
                stack[++current] = node.left_child;
                stack[++current] = node.left_child + 1;
            }
            depth++; 
        } 

    } while(current >= 0);
    return depth;
}

Ray getCameraRay(vec2 uv) {
    Ray ray;
    ray.o = u_camera.eye;
    ray.d = normalize(u_camera.dir + 
                        u_aspect_ratio * (uv.x-.5f) * cross(u_camera.dir, u_camera.up) +
                        (uv.y-.5f) * u_camera.up);
    ray.t = 1e30f;
    
    return ray;
}

void main() {
    vec2 uv = gl_FragCoord.xy / u_viewport_size;

    Ray ray = getCameraRay(uv);

    vec4 sky = vec4(70./255., 169./255., 235./255., 1.f);
    vec4 haze = vec4(vec3(0.85f), 1.);

    frag_color = mix(haze, sky, ray.d.y);
    int bvh_depth = traverseBVH(ray);
    if (bvh_depth > 0)
        frag_color = vec4(bvh_depth / (10.f * log2(bvh.length())), 0.f, 0.f, 1.f);

}
