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
        
        std::vector<Mesh>& getMeshes() { return meshes; }

        float getSceneScale() { return m_scene_scale; }

    private:
        void loadObj(std::string scene);
        void updateSceneScale();

        std::vector<Mesh> meshes;

        float m_scene_scale;
};

}
