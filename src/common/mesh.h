#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace fart {

struct AligendVertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec2 uv;
    uint32_t material_id;
};

struct Geometry {
    public:
        std::vector<AligendVertex> vertices;
        std::vector<uint32_t> indices;
};

struct Object {
    public:
        std::vector<Geometry> geometries;
};

struct ObjectInstance {
    public:
        glm::mat4 world_to_instance;
        alignas(16) uint32_t object_id;
};

}
