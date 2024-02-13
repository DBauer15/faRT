float4 tonemap_Reinhard(float4 C) {
    return C / (C + 1.f);
}

float4 tonemap_Exposure(float4 C, float exposure) {
    return float4(1.0) - exp(-C * exposure);
}

float4 tonemap_ACES(float4 C) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((C*(a*C + b)) / (C*(c*C + d) + e), 0.0f, 1.0f);
}

float4 gamma(float4 C) {
    return pow(C, float4(0.4545f)); // 1.f / 2.2f = 0.4545f
}
