vec4 tonemap_Reinhard(vec4 C) {
    return C / (C + 1.f);
}

vec4 tonemap_Exposure(vec4 C, float exposure) {
    return vec4(1.0) - exp(-C * exposure);
}

vec4 gamma(vec4 C) {
    return pow(C, vec4(0.4545f)); // 1.f / 2.2f = 0.4545f
}
