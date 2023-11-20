#include "defs.h"
#include "scene.h"


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

    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;

        // TODO: Resolve index tuples here (vertex, normal)     

    }
}

}
