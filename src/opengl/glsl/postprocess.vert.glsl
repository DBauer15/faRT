#version 450

layout (location = 0) in vec3 a_position;

out vec2 o_uv;

void main() {
    gl_Position = vec4(a_position, 1.0);
    o_uv = a_position.xy * 0.5f + 0.5f;
}
