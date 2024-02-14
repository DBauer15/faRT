#include "defs.h"
#include "scene.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <glm/ext.hpp>
#include <glm/gtx/component_wise.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <pbrtParser/Scene.h>

namespace fart {

Scene::Scene(std::string scene) {
    std::string extension = scene.substr(scene.find_last_of(".") + 1);
    if (extension == "obj") {
        loadObj(scene);
    } else if (extension == "pbrt") {
        loadPBRT(scene);
    } else {
        ERR("Unexpected file format " + extension);
        return;
    }

    updateSceneScale();
    SUCC("Finished loading " + std::to_string(m_objects.size()) + " objects and " + std::to_string(m_instances.size()) + " instances."); 
}

void
Scene::loadObj(std::string scene) {

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = scene.substr(0, scene.find_last_of('/')+1);

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(scene, reader_config)) { 
        if (!reader.Error().empty()) { 
            ERR("TinyObjLoader Error: " + reader.Error());
        }
        return;
    }
    
    if (!reader.Warning().empty()) {
        WARN("TinyObjLoader Warning: " + reader.Warning());
    }

    auto& in_attrib = reader.GetAttrib();
    auto& in_shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // Deal with normals
    bool calculate_normals = in_attrib.normals.size() == 0;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    if (calculate_normals) {
        LOG("Calculating normals");
        computeSmoothingShapes(in_attrib, attrib, in_shapes, shapes);
        computeAllSmoothingNormals(attrib, shapes);
    } else {
        LOG("Using normals provided by OBJ");
        attrib = in_attrib;
        shapes = in_shapes;
    }

    SUCC("Parsed OBJ file " + scene);

    // Parse materials and textures
    std::unordered_map<std::string, uint32_t> texture_index_map;
    for (const auto& material : materials) {
        OpenPBRMaterial pbr_mat = OpenPBRMaterial::defaultMaterial();
        pbr_mat.base_color = glm::make_vec3(material.diffuse);
        pbr_mat.specular_color = glm::make_vec3(material.specular);
        pbr_mat.specular_weight = luminance(pbr_mat.specular_color);
        pbr_mat.specular_ior = material.ior;
        pbr_mat.specular_roughness = std::clamp((1.f - std::log10(material.shininess + 1) / 3.f), 0.01f, 1.f); // TODO: This is not a good approximation of roughness as the Phong shininess is exponential
        
        if (!material.diffuse_texname.empty()) {
            if (texture_index_map.find(material.diffuse_texname) != texture_index_map.end()) {
                pbr_mat.base_color_texid = texture_index_map.at(material.diffuse_texname);
            } else {
                std::string texture_filename = reader_config.mtl_search_path + material.diffuse_texname;
                // TODO: Use std::filesystem::Path
                std::replace(texture_filename.begin(), texture_filename.end(), '\\', '/');
                Image diffuse_texture(texture_filename);
                if (diffuse_texture.isValid()) { 
                    m_textures.push_back(std::move(diffuse_texture));
                    pbr_mat.base_color_texid = m_textures.size() - 1;
                    texture_index_map[material.diffuse_texname] = m_textures.size() - 1;
                    LOG("Read texture image '" + material.diffuse_texname + "'");
                }
            }
        }

        LOG("Read material '" + material.name + "'");
        m_materials.push_back(pbr_mat);
    }
    // Add a default material for faces that do not have a material id
    m_materials.push_back(OpenPBRMaterial::defaultMaterial());

    Object obj;
    // Parse meshes
    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;

        std::map<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t> index_map; 

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

                auto key = std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);

                uint32_t g_index = 0;
                if (index_map.find(key) != index_map.end()) {
                    g_index = index_map.at(key);
                } else {
                    g_index = g_n_unique_idx_cnt++;

                    AligendVertex vertex;
                    vertex.position = glm::vec3(attrib.vertices[3 * idx.vertex_index],
                                                attrib.vertices[3 * idx.vertex_index + 1],
                                                attrib.vertices[3 * idx.vertex_index + 2]);
                    vertex.normal = glm::vec3(attrib.normals[3 * idx.normal_index],
                                              attrib.normals[3 * idx.normal_index + 1],
                                              attrib.normals[3 * idx.normal_index + 2]);
                    if (attrib.texcoords.size() > 0) {
                        vertex.uv = glm::vec2(attrib.texcoords[2 * idx.texcoord_index + 0],
                                              attrib.texcoords[2 * idx.texcoord_index + 1]);
                    }
                    vertex.material_id = mesh.material_ids[f] < 0 ? m_materials.size() - 1 : mesh.material_ids[f];

                    g.vertices.emplace_back(vertex);

                    index_map[key] = g_index;
                }
                g.indices.push_back(g_index);
            }
        }
        LOG("Read geometry (v: " + std::to_string(g.vertices.size()) + ", i: " + std::to_string(g.indices.size()) + ")");
        obj.geometries.push_back(g);
    }
    m_objects.push_back(obj);

    // OBJ does not support instancing, so each object has one instance
    for (uint32_t i = 0; i < m_objects.size(); i++) {
        ObjectInstance instance;
        instance.object_id = i;
        instance.world_to_instance = glm::inverse(glm::mat4(1.f));
        m_instances.push_back(instance);
    }
}

/*
 * Adapted from: https://github.com/tinyobjloader/tinyobjloader/blob/cc327eecf7f8f4139932aec8d75db2d091f412ef/examples/viewer/viewer.cc#L375
 */
void 
Scene::computeSmoothingShape(const tinyobj::attrib_t& in_attrib, const tinyobj::shape_t& in_shape,
                          std::vector<std::pair<unsigned int, unsigned int>>& sorted_ids,
                          unsigned int id_begin, unsigned int id_end,
                          std::vector<tinyobj::shape_t>& out_shapes,
                          tinyobj::attrib_t& out_attrib) {
  unsigned int sgroupid = sorted_ids[id_begin].first;
  bool hasmaterials = in_shape.mesh.material_ids.size();
  // Make a new shape from the set of faces in the range [id_begin, id_end).
  out_shapes.emplace_back();
  tinyobj::shape_t& outshape = out_shapes.back();
  outshape.name = in_shape.name;
  // Skip lines and points.

  std::unordered_map<unsigned int, unsigned int> remap;
  for (unsigned int id = id_begin; id < id_end; ++id) {
    unsigned int face = sorted_ids[id].second;

    outshape.mesh.num_face_vertices.push_back(3); // always triangles
    if (hasmaterials)
      outshape.mesh.material_ids.push_back(in_shape.mesh.material_ids[face]);
    outshape.mesh.smoothing_group_ids.push_back(sgroupid);
    // Skip tags.

    for (unsigned int v = 0; v < 3; ++v) {
      tinyobj::index_t inidx = in_shape.mesh.indices[3*face + v], outidx;
      assert(inidx.vertex_index != -1);
      auto iter = remap.find(inidx.vertex_index);
      // Smooth group 0 disables smoothing so no shared vertices in that case.
      if (sgroupid && iter != remap.end()) {
        outidx.vertex_index = (*iter).second;
        outidx.normal_index = outidx.vertex_index;
        outidx.texcoord_index = (inidx.texcoord_index == -1) ? -1 : outidx.vertex_index;
      }
      else {
        assert(out_attrib.vertices.size() % 3 == 0);
        unsigned int offset = static_cast<unsigned int>(out_attrib.vertices.size() / 3);
        outidx.vertex_index = outidx.normal_index = offset;
        outidx.texcoord_index = (inidx.texcoord_index == -1) ? -1 : offset;
        out_attrib.vertices.push_back(in_attrib.vertices[3*inidx.vertex_index  ]);
        out_attrib.vertices.push_back(in_attrib.vertices[3*inidx.vertex_index+1]);
        out_attrib.vertices.push_back(in_attrib.vertices[3*inidx.vertex_index+2]);
        out_attrib.normals.push_back(0.0f);
        out_attrib.normals.push_back(0.0f);
        out_attrib.normals.push_back(0.0f);
        if (inidx.texcoord_index != -1) {
          out_attrib.texcoords.push_back(in_attrib.texcoords[2*inidx.texcoord_index  ]);
          out_attrib.texcoords.push_back(in_attrib.texcoords[2*inidx.texcoord_index+1]);
        }
        remap[inidx.vertex_index] = offset;
      }
      outshape.mesh.indices.push_back(outidx);
    }
  }
}

/* 
 * Adapted from: https://github.com/tinyobjloader/tinyobjloader/blob/cc327eecf7f8f4139932aec8d75db2d091f412ef/examples/viewer/viewer.cc#L430
 */
void 
Scene::computeSmoothingShapes(const tinyobj::attrib_t& in_attrib,
                              tinyobj::attrib_t& out_attrib,
                              const std::vector<tinyobj::shape_t>& in_shapes,
                              std::vector<tinyobj::shape_t>& out_shapes) {
    for (size_t s = 0, slen = in_shapes.size() ; s < slen; ++s) {
        const tinyobj::shape_t& in_shape = in_shapes[s];

        unsigned int numfaces = static_cast<unsigned int>(in_shape.mesh.smoothing_group_ids.size());
        assert(numfaces);
        std::vector<std::pair<unsigned int,unsigned int>> sorted_ids(numfaces);
        for (unsigned int i = 0; i < numfaces; ++i)
            sorted_ids[i] = std::make_pair(in_shape.mesh.smoothing_group_ids[i], i);
        sort(sorted_ids.begin(), sorted_ids.end());

        unsigned int activeid = sorted_ids[0].first;
        unsigned int id = activeid, id_begin = 0, id_end = 0;
        // Faces are now bundled by smoothing group id, create shapes from these.
        while (id_begin < numfaces) {
            while (activeid == id && ++id_end < numfaces)
                id = sorted_ids[id_end].first;
            computeSmoothingShape(in_attrib, in_shape, sorted_ids, id_begin, id_end,
                    out_shapes, out_attrib);
            activeid = id;
            id_begin = id_end;
        }
    }
}

/* 
 * Adapted from: https://github.com/tinyobjloader/tinyobjloader/blob/cc327eecf7f8f4139932aec8d75db2d091f412ef/examples/viewer/viewer.cc#L270
 */
void 
Scene::computeAllSmoothingNormals(tinyobj::attrib_t& attrib,
                                  std::vector<tinyobj::shape_t>& shapes) {
    glm::vec3 p[3];
    for (size_t s = 0, slen = shapes.size(); s < slen; ++s) {
        const tinyobj::shape_t& shape(shapes[s]);
        size_t facecount = shape.mesh.num_face_vertices.size();
        assert(shape.mesh.smoothing_group_ids.size());

        for (size_t f = 0, flen = facecount; f < flen; ++f) {
            for (unsigned int v = 0; v < 3; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[3*f + v];
                assert(idx.vertex_index != -1);
                p[v][0] = attrib.vertices[3*idx.vertex_index  ];
                p[v][1] = attrib.vertices[3*idx.vertex_index+1];
                p[v][2] = attrib.vertices[3*idx.vertex_index+2];
            }

            // cross(p[1] - p[0], p[2] - p[0])
            float nx = (p[1][1] - p[0][1]) * (p[2][2] - p[0][2]) -
                (p[1][2] - p[0][2]) * (p[2][1] - p[0][1]);
            float ny = (p[1][2] - p[0][2]) * (p[2][0] - p[0][0]) -
                (p[1][0] - p[0][0]) * (p[2][2] - p[0][2]);
            float nz = (p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) -
                (p[1][1] - p[0][1]) * (p[2][0] - p[0][0]);

            // Don't normalize here.
            for (unsigned int v = 0; v < 3; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[3*f + v];
                attrib.normals[3*idx.normal_index  ] += nx;
                attrib.normals[3*idx.normal_index+1] += ny;
                attrib.normals[3*idx.normal_index+2] += nz;
            }
        }
    }

    assert(attrib.normals.size() % 3 == 0);
    for (size_t i = 0, nlen = attrib.normals.size() / 3; i < nlen; ++i) {
        tinyobj::real_t& nx = attrib.normals[3*i  ];
        tinyobj::real_t& ny = attrib.normals[3*i+1];
        tinyobj::real_t& nz = attrib.normals[3*i+2];
        tinyobj::real_t len = std::sqrt(nx*nx + ny*ny + nz*nz);
        tinyobj::real_t scale = len == 0 ? 0 : 1 / len;
        nx *= scale;
        ny *= scale;
        nz *= scale;
    }
}

void
Scene::updateSceneScale() {
    float min_vertex = 1e30f;
    float max_vertex = -1e30f;

    for (auto& object : m_objects) {
        for (auto& geometry : object.geometries) {
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

float
Scene::luminance(glm::vec3 c) {
    return 0.299f*c.r + 0.587f*c.g + 0.114f*c.b;
}

void
Scene::loadPBRT(std::string scene) {

    std::shared_ptr<pbrt::Scene> pbrt_scene;
    pbrt_scene = pbrt::importPBRT(scene);

    // Flatten hierarchy to avoid the pain of combining hierarchical instance transforms
    pbrt_scene->makeSingleLevel();

    // Add a default material for faces that do not have a material id
    m_materials.push_back(OpenPBRMaterial::defaultMaterial());

    // Import objects
    std::map<std::shared_ptr<pbrt::Object>, uint32_t> object_map;
    loadPBRTObjectsRecursive(pbrt_scene->world, object_map);

    // Import instances
    for(auto& instance : pbrt_scene->world->instances)
        loadPBRTInstancesRecursive(instance, object_map);
    if (m_instances.size() == 0 && m_objects.size() > 0) {
        ObjectInstance root;
        root.object_id = 0;
        root.world_to_instance = glm::inverse(glm::mat4(1.f));
        m_instances.push_back(root);
        WARN("No instance data found, adding default instance");
    }
}

void
Scene::loadPBRTObjectsRecursive(std::shared_ptr<pbrt::Object> current, std::map<std::shared_ptr<pbrt::Object>, uint32_t>& object_map) {
    if (!current || object_map.find(current) != object_map.end()) return;

    Object obj;
    for (auto& shape : current->shapes) {
        // Non-triangle shapes are not supported
        pbrt::TriangleMesh::SP mesh = std::dynamic_pointer_cast<pbrt::TriangleMesh>(shape);
        if (!mesh) continue;

        uint32_t material_id = 0;
        OpenPBRMaterial pbr_material = OpenPBRMaterial::defaultMaterial();
        if (loadPBRTMaterial(mesh->material, pbr_material)) {
            m_materials.push_back(pbr_material);
            material_id = m_materials.size() - 1;
            LOG("Parsed material '" + mesh->material->name + "'");
        }

        Geometry g;
        uint32_t g_n_idx_cnt = 0;

        for (auto& index : mesh->index) {
            for (int i = 0; i < 3; i++) {
                AligendVertex vertex;
                auto position = mesh->vertex[*(&index.x + i)];
                vertex.position = glm::vec3(position.x, position.y, position.z);

                if (mesh->normal.size() > 0) {
                    auto normal = mesh->normal[*(&index.x + i)];
                    vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
                } else {
                    const auto& v0 = glm::make_vec3(&mesh->vertex[index.x].x);
                    const auto& v1 = glm::make_vec3(&mesh->vertex[index.y].x);
                    const auto& v2 = glm::make_vec3(&mesh->vertex[index.z].x);
                    vertex.normal = glm::normalize(glm::cross((v1 - v0), (v2 - v0)));
                }

                if (mesh->texcoord.size() > 0) {
                    auto uv = mesh->texcoord[*(&index.x + i)];
                    vertex.uv = glm::vec2(uv.x, uv.y);
                }

                vertex.material_id = material_id;
                g.vertices.emplace_back(vertex);
                g.indices.emplace_back(g_n_idx_cnt++);
            }
        }
        obj.geometries.push_back(g);
        LOG("Read geometry (v: " + std::to_string(g.vertices.size()) + ", i: " + std::to_string(g.indices.size()) + ")");
    }

    if (obj.geometries.size() > 0) {
        object_map[current] = m_objects.size();
        m_objects.push_back(obj);
    }

    for (auto& instance : current->instances) {
        loadPBRTObjectsRecursive(instance->object, object_map);
    }
}

void
Scene::loadPBRTInstancesRecursive(std::shared_ptr<pbrt::Instance> current, const std::map<std::shared_ptr<pbrt::Object>, uint32_t>& object_map) {
    if (!current->object) return;

    if (object_map.find(current->object) != object_map.end()) {
        ObjectInstance instance;
        instance.object_id = object_map.at(current->object);
        auto& xfm = current->xfm;
        instance.world_to_instance = glm::inverse(glm::mat4(
                xfm.l.vx.x, xfm.l.vx.y, xfm.l.vx.z, 0,
                xfm.l.vy.x, xfm.l.vy.y, xfm.l.vy.z, 0,
                xfm.l.vz.x, xfm.l.vz.y, xfm.l.vz.z, 0,
                xfm.p.x, xfm.p.y, xfm.p.z, 1));

        m_instances.push_back(instance);
        LOG("Loaded instance of '" + current->object->name + "'");
    }

    for (auto& instance : current->object->instances) {
        loadPBRTInstancesRecursive(instance, object_map);
    }
}

bool 
Scene::loadPBRTMaterial(std::shared_ptr<pbrt::Material> material, OpenPBRMaterial& pbr_material) {
    if (std::dynamic_pointer_cast<pbrt::DisneyMaterial>(material))
        return loadPBRTMaterialDisney(*std::dynamic_pointer_cast<pbrt::DisneyMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::MixMaterial>(material))
        return loadPBRTMaterialMixMaterial(*std::dynamic_pointer_cast<pbrt::MixMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::MetalMaterial>(material))
        return loadPBRTMaterialMetal(*std::dynamic_pointer_cast<pbrt::MetalMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::TranslucentMaterial>(material))
        return loadPBRTMaterialTranslucent(*std::dynamic_pointer_cast<pbrt::TranslucentMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::PlasticMaterial>(material))
        return loadPBRTMaterialPlastic(*std::dynamic_pointer_cast<pbrt::PlasticMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::SubSurfaceMaterial>(material))
        return loadPBRTMaterialSubSurface(*std::dynamic_pointer_cast<pbrt::SubSurfaceMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::MirrorMaterial>(material))
        return loadPBRTMaterialMirror(*std::dynamic_pointer_cast<pbrt::MirrorMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::MatteMaterial>(material))
        return loadPBRTMaterialMatte(*std::dynamic_pointer_cast<pbrt::MatteMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::GlassMaterial>(material))
        return loadPBRTMaterialGlass(*std::dynamic_pointer_cast<pbrt::GlassMaterial>(material), pbr_material);
    else if (std::dynamic_pointer_cast<pbrt::UberMaterial>(material))
        return loadPBRTMaterialUber(*std::dynamic_pointer_cast<pbrt::UberMaterial>(material), pbr_material);
    else
        return false;
}

bool 
Scene::loadPBRTMaterialDisney(pbrt::DisneyMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialMixMaterial(pbrt::MixMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialMetal(pbrt::MetalMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialTranslucent(pbrt::TranslucentMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialPlastic(pbrt::PlasticMaterial& material, OpenPBRMaterial& pbr_material) {
    pbr_material.base_color = glm::make_vec3(&material.kd.x);
    pbr_material.specular_color = glm::make_vec3(&material.ks.x);
    if (material.remapRoughness) {
        float roughness = std::max(material.roughness, 1e-3f);
        float x = std::log(roughness);
        pbr_material.specular_roughness = 1.62142f + 0.819955f * x + 0.1734f * x * x +
               0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
    } else {
        pbr_material.specular_roughness = material.roughness;
    }
    return true;
}

bool 
Scene::loadPBRTMaterialSubSurface(pbrt::SubSurfaceMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialMirror(pbrt::MirrorMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialMatte(pbrt::MatteMaterial& material, OpenPBRMaterial& pbr_material) {
    pbr_material.base_color = glm::make_vec3(&material.kd.x);
    pbr_material.specular_roughness = 1.f;
    return true;
}

bool 
Scene::loadPBRTMaterialGlass(pbrt::GlassMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}

bool 
Scene::loadPBRTMaterialUber(pbrt::UberMaterial& material, OpenPBRMaterial& pbr_material) {
    return false;
}
}
