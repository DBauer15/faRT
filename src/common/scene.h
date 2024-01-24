#pragma once

#include <vector>
#include <string>

#include "mesh.h"
#include "material.h"

namespace fart {

struct Scene {

    public:
        Scene(std::string scene);
        ~Scene() = default;

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;
        
        std::vector<Mesh>& getMeshes() { return m_meshes; }
        std::vector<OpenPBRMaterial>& getMaterials() { return m_materials; }

        float getSceneScale() { return m_scene_scale; }

    private:
        void loadObj(std::string scene);
        void updateSceneScale();

        std::vector<Mesh> m_meshes;
        std::vector<OpenPBRMaterial> m_materials;

        float m_scene_scale;
};

}
