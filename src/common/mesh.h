#pragma once

#include <cstdint>
#include <vector>

namespace fart {

struct Geometry {
    public:
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<uint32_t> indices;
};

struct Mesh {
    public:
        std::vector<Geometry> geometries;
};

}
