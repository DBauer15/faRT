uniform uint u_frame_no;
uniform float u_scene_scale;
uniform uvec2 u_viewport_size;
uniform float u_aspect_ratio;
uniform Camera u_camera;

layout(std430, binding = 0) buffer geometry0 {
    Vertex vertices [];
};

layout(std430, binding = 1) buffer geometry1 {
    uint indices [];
};

layout(std430, binding = 2) buffer accel0 {
    BVHNode bvh [];
};

layout(std430, binding = 3) buffer accel1 {
    TLASNode tlas [];
};

layout(std430, binding = 4) buffer inst0 {
    Instance instances [];
};

layout(std430, binding = 5) buffer mat0 {
    OpenPBRMaterial materials [];
};

layout(std430, binding = 6) buffer tex0 {
    sampler2D textures [];
};
