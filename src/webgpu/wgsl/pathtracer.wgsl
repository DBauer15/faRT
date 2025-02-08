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

struct RNG {
    state: u32,
};

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


/* Data */
@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var dst_texture: texture_storage_2d<rgba16float, write>;

/* Pathtracer */
fn intersect(ray: Ray) -> SurfaceInteraction {
    var si: SurfaceInteraction;

    if (dot(ray.d, vec3f(0.0, 0.0, 1.0)) > 0.95) {
        si.valid = true;
    }
    return si;
}

fn miss(ray: Ray) -> vec4f {
    let sky = vec4f(70./255., 169./255., 235./255., 1.0);
    let haze = vec4f(127./255., 108./255., 94./255., 1.0);
    let background = mix(sky, haze, (ray.d.y + 1.0) /2.0);
    return background;
}

fn closestHit(si: SurfaceInteraction) -> vec4f {
    let color = vec4f(1.0, 0.0, 0.0, 1.0);

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
                   f32(id.y) / f32(uniforms.viewport_size.y));
    let d = uv + (next_random2f(&rng) / vec2f(uniforms.viewport_size.xy));
    let ray = spawnRay(uniforms, d);

    let si = intersect(ray);
    if (si.valid) {
        L = closestHit(si);
    } else {
        L = miss(ray);
    }

    textureStore(dst_texture, id.xy, L);
}