#version 450

in vec3 o_normal;

out vec4 glFragColor;

void main() {
    glFragColor = vec4(o_normal, 1.f);
}
