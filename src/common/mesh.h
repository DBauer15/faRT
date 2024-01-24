#pragma once

#include <cstdint>
#include <vector>
#include "glm/glm.hpp"

namespace fart {

struct alignas(16) AligendVertex {
    glm::vec3 position;
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
