#pragma once

#include <vector>

namespace fart {

struct Shape {
    public:
        std::vector<float> vertices;
        std::vector<float> normals;

};

struct Mesh {
    public:
        std::vector<Shape> shapes;
};

}
