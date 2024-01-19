#define EPS 0.00000001

bool intersectTriangle(inout Ray ray, inout SurfaceInteraction si, uint first_index) {
    vec3 v0 = vertices[indices[first_index+0]].xyz;
    vec3 v1 = vertices[indices[first_index+1]].xyz;
    vec3 v2 = vertices[indices[first_index+2]].xyz;

    const vec3 edge1 = v1 - v0;
    const vec3 edge2 = v2 - v0;
    const vec3 h = cross( ray.d, edge2 );
    const float a = dot( edge1, h );
    if (a > -EPS && a < EPS) return false; // ray parallel to triangle
    const float f = 1 / a;
    const vec3 s = ray.o - v0;
    const float u = f * dot( s, h );
    if (u < 0 || u > 1) return false;
    const vec3 q = cross( s, edge1 );
    const float v = f * dot( ray.d, q );
    if (v < 0 || u + v > 1) return false;
    const float t = f * dot( edge2, q );
    if (t > EPS) {
        // update ray and SurfaceInteraction
        // TODO: This could be moved to somewhere nicer with less divergence
        if (t < ray.t) {
            si.n = normalize(cross(edge1, edge2));
            si.n = si.n * sign(dot(si.n, -ray.d));
        }
        ray.t = min( ray.t, t );
        si.valid = true;
        return true;
    }
    return false;
}

float intersectAABB(Ray ray, BVHNode node) {
    vec3 bmin = node.aabb_min.xyz;
    vec3 bmax = node.aabb_max.xyz;

    float tx1 = (bmin.x - ray.o.x) * ray.rD.x, tx2 = (bmax.x - ray.o.x) * ray.rD.x;
    float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
    float ty1 = (bmin.y - ray.o.y) * ray.rD.y, ty2 = (bmax.y - ray.o.y) * ray.rD.y;
    tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
    float tz1 = (bmin.z - ray.o.z) * ray.rD.z, tz2 = (bmax.z - ray.o.z) * ray.rD.z;
    tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );

    if (tmax >= tmin && tmin < ray.t && tmax > 0) return tmin;
    return 1e30f; 
}

SurfaceInteraction intersect(Ray ray) {
    SurfaceInteraction si;
    si.valid = false;
    si.mat = OpenPBRMaterial( 1.f, vec3(0.85f, 0.75f, 0.75f), 1.f, 0.f );

    uint stack[64];
    int current = 0;
    stack[current] = 0;

    do {
        BVHNode node = bvh[stack[current--]];

        if (node.left_child <= 0) {
            // intersect triangles in the node
            for (int i = 0; i < node.tri_count; i++) {
                intersectTriangle(ray, si, node.first_tri_index_id + (3*i));
            }
        } else {
            float left_dist = intersectAABB(ray, bvh[node.left_child]);
            float right_dist = intersectAABB(ray, bvh[node.left_child+1]);

            if (left_dist > right_dist) {
                if (left_dist < 1e30f) stack[++current] = node.left_child;
                if (right_dist < 1e30f) stack[++current] = node.left_child+1;
            } else {
                if (right_dist < 1e30f) stack[++current] = node.left_child+1;
                if (left_dist < 1e30f) stack[++current] = node.left_child;
            }
        }
    } while(current >= 0 && current < 64);

    si.p = ray.o + ray.d * ray.t;
    si.w_o = -ray.d;
    
    return si;
}
