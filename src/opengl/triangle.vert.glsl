#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

out vec3 o_normal;

uniform mat4 u_vp;

void main() {
    gl_Position = u_vp * vec4(a_position, 1.f);
    o_normal = a_normal;
}
