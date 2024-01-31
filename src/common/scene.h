#pragma once

#include <vector>
#include <string>

#include "mesh.h"
#include "material.h"
#include "image.h"

// foward declaration for method signatures in Scene
namespace tinyobj {
    struct attrib_t;
    struct shape_t;
}

namespace fart {

struct Scene {

    public:
        Scene(std::string scene);
        ~Scene() = default;

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;
        
        std::vector<Mesh>& getMeshes() { return m_meshes; }
        std::vector<OpenPBRMaterial>& getMaterials() { return m_materials; }
        std::vector<Image>& getTextures() { return m_textures; }

        float getSceneScale() { return m_scene_scale; }

    private:
        /* OBJ Parsing */
        void loadObj(std::string scene);
        void computeSmoothingShape(const tinyobj::attrib_t& in_attrib, const tinyobj::shape_t& in_shape,
                                  std::vector<std::pair<unsigned int, unsigned int>>& sorted_ids,
                                  unsigned int id_begin, unsigned int id_end,
                                  std::vector<tinyobj::shape_t>& out_shapes,
                                  tinyobj::attrib_t& out_attrib);
        void computeSmoothingShapes(const tinyobj::attrib_t& in_attrib,
                                    tinyobj::attrib_t& out_attrib,
                                    const std::vector<tinyobj::shape_t>& in_shapes,
                                    std::vector<tinyobj::shape_t>& out_shapes);
        void computeAllSmoothingNormals(tinyobj::attrib_t& attrib,
                                        std::vector<tinyobj::shape_t>& shapes);


        void updateSceneScale();

        std::vector<Mesh> m_meshes;
        std::vector<OpenPBRMaterial> m_materials;
        std::vector<Image> m_textures;

        float m_scene_scale;
};

}
