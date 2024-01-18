#pragma once

#include <cstdint>
#include <vector>
#include "glm/glm.hpp"

namespace fart {

// TODO Use this type in the geometry definition instead of raw floats
struct AligendVertex {
    alignas(16) glm::vec3 position;
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
