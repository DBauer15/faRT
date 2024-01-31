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

struct Mesh {
    public:
        std::vector<Geometry> geometries;
};

}
