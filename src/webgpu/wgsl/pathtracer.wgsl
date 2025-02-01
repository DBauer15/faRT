@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;


@compute @workgroup_size(32)
fn pathtracer(@builtin(global_invocation_id) id: vec3<u32>) {
    outputBuffer[id.x] = 2.0 * inputBuffer[id.x] + 1.0;
}