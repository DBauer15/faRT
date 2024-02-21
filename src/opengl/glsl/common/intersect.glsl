vec3 getNormal(uint first_index, vec3 bary) {
    vec3 n0 = vertices[indices[first_index+0]].normal.xyz;
    vec3 n1 = vertices[indices[first_index+1]].normal.xyz;
    vec3 n2 = vertices[indices[first_index+2]].normal.xyz;

    return normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);
}

vec2 getUV(uint first_index, vec3 bary) {
    vec2 uv0 = vertices[indices[first_index+0]].uv.xy;
    vec2 uv1 = vertices[indices[first_index+1]].uv.xy;
    vec2 uv2 = vertices[indices[first_index+2]].uv.xy;

    return uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;
}

bool intersectTriangle(inout Ray ray, inout SurfaceInteraction si, uint first_index) {
    uint material_id = vertices[indices[first_index+0]].material_id;
    vec3 v0 = vertices[indices[first_index+0]].position.xyz;
    vec3 v1 = vertices[indices[first_index+1]].position.xyz;
    vec3 v2 = vertices[indices[first_index+2]].position.xyz;

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
            vec3 bary = vec3(1.f - u - v, u, v);
            vec2 uv = getUV(first_index, bary);
            OpenPBRMaterial mat = materials[material_id];
            vec3 face_normal = normalize(cross(edge1, edge2));
            vec3 vertex_normal = getNormal(first_index, bary);
            vertex_normal = vertex_normal * (dot(face_normal, -ray.d) < 0.f ? -1.f : 1.f);
            face_normal = face_normal * (dot(face_normal, -ray.d) < 0.f ? -1.f : 1.f);

            if (mat.base_color_texid >= 0 && texture(u_textures[mat.base_color_texid], uv).a == 0.f) return false;
            si.uv = uv;
            si.n = vertex_normal;
            si.mat = mat;
            ray.t = min( ray.t, t );

            si.w_o = normalize(-ray.d);
            si.valid = true;
            return true;
        }
    }
    return false;
}

float intersectAABB(Ray ray, vec3 bmin, vec3 bmax) {
    float tx1 = (bmin.x - ray.o.x) * ray.rD.x, tx2 = (bmax.x - ray.o.x) * ray.rD.x;
    float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
    float ty1 = (bmin.y - ray.o.y) * ray.rD.y, ty2 = (bmax.y - ray.o.y) * ray.rD.y;
    tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
    float tz1 = (bmin.z - ray.o.z) * ray.rD.z, tz2 = (bmax.z - ray.o.z) * ray.rD.z;
    tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );

    if (tmax >= tmin && tmin < ray.t && tmax > 0) return tmin;
    return 1e30f; 
}

void intersectBLAS(inout Ray ray, inout SurfaceInteraction si, uint bvh_offset) {
    uint stack[32];
    int current = 0;
    stack[current] = bvh_offset;

    do {
        BVHNode node = bvh[stack[current--]];

        if (node.left_child <= 0) {
            // intersect triangles in the node
            for (int i = 0; i < node.tri_count; i++) {
                intersectTriangle(ray, si, node.first_tri_index_id + (3*i));
            }
        } else {
            float left_dist = intersectAABB(ray, bvh[bvh_offset + node.left_child].aabb_min.xyz, bvh[bvh_offset + node.left_child].aabb_max.xyz);
            float right_dist = intersectAABB(ray, bvh[bvh_offset + node.left_child+1].aabb_min.xyz, bvh[bvh_offset + node.left_child+1].aabb_max.xyz);

            if (left_dist > right_dist) {
                if (left_dist < 1e30f) stack[++current] = bvh_offset + node.left_child;
                if (right_dist < 1e30f) stack[++current] = bvh_offset + node.left_child+1;
            } else {
                if (right_dist < 1e30f) stack[++current] = bvh_offset + node.left_child+1;
                if (left_dist < 1e30f) stack[++current] = bvh_offset + node.left_child;
            }
        }
        
    } while(current >= 0 && current < 32);

}

SurfaceInteraction intersect(Ray ray) {
    SurfaceInteraction si;
    si.valid = false;

    uint stack[32];
    int current = 0;
    stack[current] = 0;

    do {
        TLASNode node = tlas[stack[current--]];
        if (node.left == 0 && node.right == 0) {
            Ray ray_backup = ray;
            mat4 xfm = instances[node.instance].world_to_instance;
            ray.o = vec3(xfm * vec4(ray.o, 1));
            ray.d = vec3(xfm * vec4(ray.d, 0));
            ray.rD = 1.f / ray.d;

            intersectBLAS(ray, si, node.blas);
            ray_backup.t = ray.t;
            ray = ray_backup;

        } else {
            float left_dist = intersectAABB(ray, tlas[node.left].aabb_min.xyz, tlas[node.left].aabb_max.xyz);
            float right_dist = intersectAABB(ray, tlas[node.right].aabb_min.xyz, tlas[node.right].aabb_max.xyz);

            if (left_dist > right_dist) {
                if (left_dist < 1e30f) stack[++current] = node.left;
                if (right_dist < 1e30f) stack[++current] = node.right;
            } else {
                if (right_dist < 1e30f) stack[++current] = node.right;
                if (left_dist < 1e30f) stack[++current] = node.left;
            }
        }
    } while (current >= 0 && current < 32);

    si.p = ray.o + ray.d * ray.t;
    return si;
}
