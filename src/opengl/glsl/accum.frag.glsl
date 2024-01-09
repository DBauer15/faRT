#version 450

uniform sampler2D u_framebuffer;
uniform sampler2D u_framebuffer_accum;

in vec2 o_uv;
out vec4 frag_color;

int main() {
    frag_color = texture(u_framebuffer, o_uv);
}
