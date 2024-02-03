#pragma once

#include <vector>
#include <string>
#include <memory>


#include "mesh.h"
#include "material.h"
#include "image.h"

// foward declaration for method signatures in Scene
namespace tinyobj {
    struct attrib_t;
    struct shape_t;
}

namespace pbrt {
    struct Material;
    struct DisneyMaterial;
    struct MixMaterial;
    struct MetalMaterial;
    struct TranslucentMaterial;
    struct PlasticMaterial;
    struct SubSurfaceMaterial;
    struct MirrorMaterial;
    struct MatteMaterial;
    struct GlassMaterial;
    struct UberMaterial;
}

namespace fart {

struct Scene {

    public:
        Scene(std::string scene);
        ~Scene() = default;

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;
        
        std::vector<Object>& getObjects() { return m_objects; }
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

        /* PBRT Parsing */
        void loadPBRT(std::string scene);
        bool loadPBRTMaterial(std::shared_ptr<pbrt::Material> material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialDisney(pbrt::DisneyMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialMixMaterial(pbrt::MixMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialMetal(pbrt::MetalMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialTranslucent(pbrt::TranslucentMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialPlastic(pbrt::PlasticMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialSubSurface(pbrt::SubSurfaceMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialMirror(pbrt::MirrorMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialMatte(pbrt::MatteMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialGlass(pbrt::GlassMaterial& material, OpenPBRMaterial& pbr_material);
        bool loadPBRTMaterialUber(pbrt::UberMaterial& material, OpenPBRMaterial& pbr_material);

        /* Utility Functions */
        void updateSceneScale();

        /* Scene Data */
        std::vector<Object> m_objects;
        std::vector<ObjectInstance> m_instances;
        std::vector<OpenPBRMaterial> m_materials;
        std::vector<Image> m_textures;

        float m_scene_scale;
};

}
