@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;

@group(0) @binding(2) var src_texture: texture_storage_2d<rgba16float, read>;
@group(0) @binding(3) var dst_texture: texture_storage_2d<rgba16float, write>;

@compute @workgroup_size(32, 32)
fn pathtracer(@builtin(global_invocation_id) id: vec3<u32>) {
    outputBuffer[id.x] = 2.0 * inputBuffer[id.x] + 1.0;
    let color = vec4f(abs(sin(f32(id.x) * 0.01)), abs(cos(f32(id.y) * 0.01)), 0.2, 1.0);
    textureStore(dst_texture, id.xy, color);
}