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
    let background = mix(sky, haze, (ray.d.x + 1.0) /2.0);
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

    var L = vec4f(0.0);
    let uv = vec2f(f32(id.x) / f32(uniforms.viewport_size.x),
                   f32(id.y) / f32(uniforms.viewport_size.y));
    let d = uv; /* TODO: jitter */
    let ray = spawnRay(uniforms, d);

    let si = intersect(ray);
    if (si.valid) {
        L = closestHit(si);
    } else {
        L = miss(ray);
    }

    textureStore(dst_texture, id.xy, L);
}