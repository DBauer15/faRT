vec4 tonemap_Reinhard(vec4 C) {
    return C / (C + 1.f);
}

vec4 tonemap_Exposure(vec4 C, float exposure) {
    return vec4(1.0) - exp(-C * exposure);
}

vec4 tonemap_ACES(vec4 C) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((C*(a*C + b)) / (C*(c*C + d) + e), 0.0f, 1.0f);
}

vec4 gamma(vec4 C) {
    return pow(C, vec4(0.4545f)); // 1.f / 2.2f = 0.4545f
}
