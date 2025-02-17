/* Constants */
const EPS = 1e-5;
/* Constants */

/* Types */
struct Uniforms {
    frame_number: u32,
    scene_scale: f32,
    aspect_ratio: f32,
    eye: vec3f,
    dir: vec3f,
    up:  vec3f,
    viewport_size: vec3u,
};

struct Vertex {
    position: vec3f,
    normal: vec3f, 
    uv: vec2f,
    material_id: u32,
};

struct Ray {
    o: vec3f,
    d: vec3f,
    rD: vec3f,
    t: f32,
};

struct SurfaceInteraction {
    p: vec3f,
    n: vec3f,
    w_i: vec3f,
    w_o: vec3f,
    uv: vec2f,
    valid: bool,
};

struct BVHNode {
    left_child: u32,
    first_tri_index_id: u32,
    tri_count: u32,
    aabb_min: vec3f,
    aabb_max: vec3f,
};

struct RNG {
    state: u32,
};
/* Types */

/* Data */
@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<storage,read> vertices: array<Vertex>;
@group(0) @binding(2) var<storage,read> indices: array<u32>;
@group(0) @binding(3) var<storage,read> bvh: array<BVHNode>;
@group(0) @binding(4) var dst_texture: texture_storage_2d<rgba16float, write>;
/* Data */

/* Random */
fn murmurhash3_mix(hash: u32, k: u32) -> u32 {
    let c1: u32 = 0xcc9e2d51u;
    let c2: u32 = 0x1b873593u;
    let r1: u32 = 15u;
    let r2: u32 = 13u;
    let m: u32 = 5u;
    let n: u32 = 0xe6546b64u;

    var _k = k * c1;
    _k = (_k << r1) | (_k >> (32 - r1));
    _k *= c2;

    var _hash = hash ^ _k;
    _hash = ((_hash << r2) | (_hash >> (32 - r2))) * m + n;
    
    return _hash;
}

fn murmurhash3_finalize(hash: u32) -> u32 {
    var _hash = hash;
    _hash ^= _hash >> 16;
    _hash *= 0x85ebca6bu;
    _hash ^= _hash >> 13;
    _hash *= 0xc2b2ae35u;
    _hash ^= _hash >> 16;

    return _hash;
}

fn make_random(pixel_id: u32, frame_no: u32) -> RNG {
    var rng: RNG;
    rng.state = murmurhash3_mix(0u, pixel_id);
    rng.state = murmurhash3_mix(rng.state, frame_no);
    rng.state = murmurhash3_finalize(rng.state);

    return rng;
}

fn next_random(rng: ptr<function,RNG>) -> u32 {
    (*rng).state = 1664525u * (*rng).state + 1013904223u;
    return (*rng).state;
}

fn next_randomf(rng: ptr<function,RNG>) -> f32 {
    let r = next_random(rng);
    return bitcast<f32>((r & 0x007FFFFFu) | 0x3F800000u) - 1.0;
}

fn next_random2f(rng: ptr<function,RNG>) -> vec2f {
    return vec2f(next_randomf(rng), next_randomf(rng));
}

fn next_random3f(rng: ptr<function,RNG>) -> vec3f {
    return vec3f(next_randomf(rng), next_randomf(rng), next_randomf(rng));
}
/* Random */

/* Intersect */
fn getNormal(first_index: u32, bary: vec3f) -> vec3f {
    let n0: vec3f = vertices[indices[first_index+0]].normal.xyz;
    let n1: vec3f = vertices[indices[first_index+1]].normal.xyz;
    let n2: vec3f = vertices[indices[first_index+2]].normal.xyz;

    return normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);
}

fn getUV(first_index: u32, bary: vec3f) -> vec2f {
    let uv0: vec2f = vertices[indices[first_index+0]].uv.xy;
    let uv1: vec2f = vertices[indices[first_index+1]].uv.xy;
    let uv2: vec2f = vertices[indices[first_index+2]].uv.xy;

    return uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;
}


fn intersectTriangle(ray: ptr<function,Ray>, 
                     si: ptr<function,SurfaceInteraction>,
                     first_index: u32) -> bool
{
    let material_id = vertices[indices[first_index+0]].material_id;
    let v0 = vertices[indices[first_index+0]].position.xyz;
    let v1 = vertices[indices[first_index+1]].position.xyz;
    let v2 = vertices[indices[first_index+2]].position.xyz;

    let edge1 = v1 - v0;
    let edge2 = v2 - v0;
    let h = cross( (*ray).d, edge2 );
    let a = dot( edge1, h );
    if (a > -EPS && a < EPS) {
        return false; // ray parallel to triangle
    }
    let f = 1 / a;
    let s = (*ray).o - v0;
    let u = f * dot( s, h );
    if (u < 0 || u > 1) {
        return false;
    }
    let q = cross( s, edge1 );
    let v = f * dot( (*ray).d, q );
    if (v < 0 || u + v > 1) {
        return false;
    }
    let t = f * dot( edge2, q );
    if (t > EPS) {
        // update ray and SurfaceInteraction
        // TODO: This could be moved to somewhere nicer with less divergence
        if (t < (*ray).t) {
            let bary = vec3f(1.0 - u - v, u, v);
            let uv = getUV(first_index, bary);
            // OpenPBRMaterial mat = materials[material_id];
            var face_normal = normalize(cross(edge1, edge2));
            var vertex_normal = getNormal(first_index, bary);
            if (dot(face_normal, -(*ray).d) < 0.0) {
                vertex_normal = vertex_normal * -1.0;
            }
            if (dot(face_normal, -(*ray).d) < 0.0) {
                face_normal = face_normal * -1.0;
            }

            // if (mat.base_color_texid >= 0 && texture(textures[mat.base_color_texid], uv).a < 0.001f) return false;
            (*si).uv = uv;
            (*si).n = vertex_normal;
            // si.mat = mat;
            (*ray).t = min( (*ray).t, t );


            (*si).valid = true;
            return true;
        }
    }
    return false;
}

fn intersectAABB(ray: ptr<function,Ray>, bmin: vec3f, bmax: vec3f) -> f32 {
    let tx1: f32 = (bmin.x - (*ray).o.x) * (*ray).rD.x; 
    let tx2: f32 = (bmax.x - (*ray).o.x) * (*ray).rD.x;
    var tmin: f32 = min( tx1, tx2 ); 
    var tmax: f32 = max( tx1, tx2 );
    let ty1: f32 = (bmin.y - (*ray).o.y) * (*ray).rD.y; 
    let ty2: f32 = (bmax.y - (*ray).o.y) * (*ray).rD.y;

    tmin = max( tmin, min( ty1, ty2 ) ); 
    tmax = min( tmax, max( ty1, ty2 ) );

    let tz1: f32 = (bmin.z - (*ray).o.z) * (*ray).rD.z;
    let tz2: f32 = (bmax.z - (*ray).o.z) * (*ray).rD.z;
    tmin = max( tmin, min( tz1, tz2 ) );
    tmax = min( tmax, max( tz1, tz2 ) );

    if (tmax >= tmin && tmin < (*ray).t && tmax > 0.0) { 
        return tmin;
    }

    return 1e30; 
}

fn intersectBLAS(ray: ptr<function,Ray>, si: ptr<function,SurfaceInteraction>, bvh_offset: u32) {
    var stack = array<u32, 16>();
    var current: i32 = 0;
    stack[current] = bvh_offset;

    loop {
        var node: BVHNode = bvh[stack[current]];
        current = current - 1;

        if (node.left_child <= 0) {
            // intersect triangles in the node
            for (var i: u32 = 0; i < node.tri_count; i = i+1) {
            //for (var i: u32 = 0; i < 4; i = i+1) {
                intersectTriangle(ray, si, node.first_tri_index_id + (3*i));
            }
        } else {
            let left_dist: f32 = intersectAABB(ray, bvh[bvh_offset + node.left_child].aabb_min.xyz, bvh[bvh_offset + node.left_child].aabb_max.xyz);
            let right_dist: f32 = intersectAABB(ray, bvh[bvh_offset + node.left_child+1].aabb_min.xyz, bvh[bvh_offset + node.left_child+1].aabb_max.xyz);


            if (left_dist > right_dist) {
                if (left_dist < 1e30) { 
                    current += 1;
                    stack[current] = bvh_offset + node.left_child;
                }
                if (right_dist < 1e30) {
                    current += 1;
                    stack[current] = bvh_offset + node.left_child+1;
                }
            } else {
                if (right_dist < 1e30) {
                    current += 1;
                    stack[current] = bvh_offset + node.left_child+1;
                }
                if (left_dist < 1e30) {
                    current += 1;
                    stack[current] = bvh_offset + node.left_child;
                }
            }
        }

        if (current < 0 || current >= 15) {
            return;
        }
    }
}

fn intersect(ray: ptr<function,Ray>) -> SurfaceInteraction {
    var si: SurfaceInteraction;

    intersectBLAS(ray, &si, 0u);

    return si;
}
/* Intersect */

/* Pathtracer */
fn miss(ray: Ray) -> vec4f {
    let sky = vec4f(70./255., 169./255., 235./255., 1.0);
    let haze = vec4f(127./255., 108./255., 94./255., 1.0);
    let background = mix(haze, sky, (ray.d.y + 1.0) /2.0);
    return background;
}

fn closestHit(si: SurfaceInteraction) -> vec4f {
    let ambient = vec3f(0.2);
    let diffuse = dot(vec3f(0, 1, 0), si.n) * vec3f(0.9, 0.8, 0.6);
    let color = vec4f(ambient + diffuse, 1.0);

    return color;
}

fn spawnRay(uniforms: Uniforms, d: vec2f) -> Ray {
    let right: vec3f = normalize(cross(uniforms.dir, uniforms.up));
    var ray: Ray;
    ray.o = uniforms.eye;
    ray.d = normalize(uniforms.dir +
                      uniforms.aspect_ratio * (d.x - 0.5) * right +
                      (d.y - 0.5) * uniforms.up);
    ray.rD = 1.0 / ray.d;
    ray.t = 1e30;
    return ray;
}

@compute @workgroup_size(32, 32)
fn pathtracer(@builtin(global_invocation_id) id: vec3<u32>) {
    let pixel_id = u32(id.y * uniforms.viewport_size.x + id.x);
    var rng = make_random(pixel_id, uniforms.frame_number);

    var L = vec4f(0.0);
    let uv = vec2f(f32(id.x) / f32(uniforms.viewport_size.x),
                   1.0 - (f32(id.y) / f32(uniforms.viewport_size.y)));
    let d = uv + (next_random2f(&rng) / vec2f(uniforms.viewport_size.xy));
    var ray = spawnRay(uniforms, d);

    let si = intersect(&ray);
    if (si.valid) {
        L = closestHit(si);
    } else {
        L = miss(ray);
    }

    textureStore(dst_texture, id.xy, L);
}
/* Pathtracer */
