#include "defs.h"
#include "mesh.h"
#include "scene.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
namespace fart {

Scene::Scene(std::string scene) {
    std::string extension = scene.substr(scene.find_last_of(".") + 1);
    if (extension == "obj") {
        load_obj(scene);
    } else 
        throw std::runtime_error("Unexpected file format " + extension);

   SUCC("Finished loading " + std::to_string(meshes.size()) + " meshes."); 
}

void
Scene::load_obj(std::string scene) {

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

                    g.vertices.emplace_back(attrib.vertices[3 * idx.vertex_index]);
                    g.vertices.emplace_back(attrib.vertices[3 * idx.vertex_index + 1]);
                    g.vertices.emplace_back(attrib.vertices[3 * idx.vertex_index + 2]);

                    if (idx.normal_index != uint32_t(-1)) {
                        g.normals.emplace_back(attrib.normals[3 * idx.normal_index]);
                        g.normals.emplace_back(attrib.normals[3 * idx.normal_index + 1]);
                        g.normals.emplace_back(attrib.normals[3 * idx.normal_index + 2]);
                    }

                    index_map[key] = g_index;
                }
                g.indices.push_back(g_index);
            }
        }
        SUCC("Read geometry (v: " + std::to_string(g.vertices.size()) + ", i: " + std::to_string(g.indices.size()) + ", n: " + std::to_string(g.normals.size()) + ")");
        m.geometries.push_back(g);
    }
    meshes.push_back(m);
}

}
