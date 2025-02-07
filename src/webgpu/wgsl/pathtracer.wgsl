struct Uniforms {
    frame_number: u32,
    scene_scale: f32,
    aspect_ratio: f32,
    eye: vec3f,
    dir: vec3f,
    up:  vec3f,
    viewport_size: vec2f,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(0) var<storage,read_write> outputBuffer: array<f32,64>;

@group(0) @binding(2) var src_texture: texture_storage_2d<rgba16float, read>;
@group(0) @binding(3) var dst_texture: texture_storage_2d<rgba16float, write>;

@compute @workgroup_size(32, 32)
fn pathtracer(@builtin(global_invocation_id) id: vec3<u32>) {
    let color = vec4f(uniforms.dir, 1.0);
    textureStore(dst_texture, id.xy, color);
}