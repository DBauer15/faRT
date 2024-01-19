#include "defs.h"
#include "mesh.h"
#include "scene.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <glm/gtx/component_wise.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace fart {

Scene::Scene(std::string scene) {
    std::string extension = scene.substr(scene.find_last_of(".") + 1);
    if (extension == "obj") {
        loadObj(scene);
    } else 
        throw std::runtime_error("Unexpected file format " + extension);

    updateSceneScale();
    SUCC("Finished loading " + std::to_string(meshes.size()) + " meshes."); 
}

void
Scene::loadObj(std::string scene) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, scene.c_str());

    if (!ok || !err.empty()) {
        ERR("TinyObjLoader Error: " + err);
        return;
    }

    if (!warn.empty())
        WARN("TinyObjLoader Warning: " + warn);

    SUCC("Parsed OBJ file " + scene);

    Mesh m;
    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;

        // TODO: Resolve index tuples here (vertex, normal)     
        std::map<std::tuple<uint32_t, uint32_t>, uint32_t> index_map; 

        Geometry g;

        // Keep track of all the unique indices we use
        uint32_t g_n_unique_idx_cnt = 0;

        // Loop over faces in the mesh
        for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(mesh.num_face_vertices[f]);

            if (fv != 3) {
                ERR("Found non-triangular primitive with " + std::to_string(fv) + " vertices.");
                return;
            }

            // Loop over vertices in the face
            for (size_t v = 0; v < fv; v++){

                tinyobj::index_t idx = mesh.indices[3 * f + v]; 

                auto key = std::make_tuple(idx.vertex_index, idx.normal_index);

                uint32_t g_index = 0;
                if (index_map.find(key) != index_map.end()) {
                    g_index = index_map.at(key);
                } else {
                    g_index = g_n_unique_idx_cnt++;

                    AligendVertex vertex;
                    vertex.position = glm::vec3(attrib.vertices[3 * idx.vertex_index],
                                                attrib.vertices[3 * idx.vertex_index + 1],
                                                attrib.vertices[3 * idx.vertex_index + 2]);

                    g.vertices.emplace_back(vertex);

                    index_map[key] = g_index;
                }
                g.indices.push_back(g_index);
            }
        }
        SUCC("Read geometry (v: " + std::to_string(g.vertices.size()) + ", i: " + std::to_string(g.indices.size()) + ")");
        m.geometries.push_back(g);
    }
    meshes.push_back(m);
}

void
Scene::updateSceneScale() {
    float min_vertex = 1e30f;
    float max_vertex = -1e30f;

    for (auto& mesh : meshes) {
        for (auto& geometry : mesh.geometries) {
            min_vertex = std::min(min_vertex, glm::compMin(std::min_element(geometry.vertices.begin(), geometry.vertices.end(),
                            [](auto v0, auto v1) {
                                return glm::compMin(v0.position) < glm::compMin(v1.position);
                            })->position));

            max_vertex = std::max(min_vertex, glm::compMax(std::max_element(geometry.vertices.begin(), geometry.vertices.end(),
                            [](auto v0, auto v1) {
                                return glm::compMax(v0.position) < glm::compMax(v1.position);
                            })->position));
        }
    }

    m_scene_scale = max_vertex - min_vertex;
}

}
