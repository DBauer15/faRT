#include "mesh.h"

namespace fart {

OptixGeometry::OptixGeometry(const Geometry& geometry) {
    m_vertex_size = geometry.positions.stride();
    m_vertices.alloc_and_upload(geometry.positions.data(), geometry.positions.size() * geometry.positions.stride());
    m_indices.alloc_and_upload(geometry.indices);
}

OptixGeometrySBTData
OptixGeometry::getSBTData() {
    OptixGeometrySBTData data;
    data.vertices = (Geometry*)m_vertices.device_ptr();
    data.indices = (uint32_t*)m_indices.device_ptr();
}

OptixObject::OptixObject(const Object& object) {
    for (auto& geometry : object.geometries) {
        OptixGeometry g(geometry);
        m_geometries.push_back(g);
    }
}

}
