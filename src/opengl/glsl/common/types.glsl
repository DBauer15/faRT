struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
    uint material_id;
};

struct Camera {
    vec3 eye;
    vec3 dir;
    vec3 up;
};

struct Ray {
    vec3 o;
    vec3 d;
    vec3 rD;
    float t;
};

struct OpenPBRMaterial {
    vec3  base_color;
    int   base_color_texid;
    float base_weight;
    float base_roughness;
    float base_metalness;

    vec3  specular_color;
    float specular_weight;
    float specular_roughness;
    float specular_anisotropy;
    float specular_rotation;
    float specular_ior;
    float specular_ior_level;

    float transmission_weight;

    float geometry_opacity;
    int   geometry_opacity_texid;
};

struct SurfaceInteraction {
    vec3 p;
    vec3 n;
    vec3 w_i;
    vec3 w_o;
    vec2 uv;
    OpenPBRMaterial mat;
    bool valid;
};

struct BVHNode {
    vec4 aabb_min;
    vec4 aabb_max;
    uint left_child;
    uint first_tri_index_id;
    uint tri_count;
    uint filler; // TODO: Figure out a better way to deal with alignments
};

struct TLASNode {
    vec4 aabb_min;
    vec4 aabb_max;
    uint blas;
    uint instance;
    uint left;
    uint right;
};

struct Instance {
    mat4 world_to_instance;
    uint object_id;
};

struct RNG {
    uint state;
};
