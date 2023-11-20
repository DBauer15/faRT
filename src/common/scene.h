#pragma once

#include <vector>
#include <string>

#include "mesh.h"

namespace fart {

struct Scene {

    public:
        Scene(std::string scene);
        ~Scene() = default;

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;

    private:
        void load_obj(std::string scene);
        std::vector<Mesh> meshes;
};

}
