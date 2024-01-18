#version 450
#extension GL_GOOGLE_include_directive : enable 

#include "common/color.glsl"

uniform sampler2D u_frag_color_accum;
in vec2 o_uv;

out vec4 frag_color;

void main() {
    vec4 c = texture(u_frag_color_accum, o_uv);
    frag_color = gamma(tonemap_ACES(c));
}
