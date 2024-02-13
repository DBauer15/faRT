#include "mesh.h"


namespace fart {

MetalGeometry::MetalGeometry(MTL::Device* device, const Geometry& geometry) {
    m_vertices = device->newBuffer(geometry.vertices.data(), 
                                   geometry.vertices.size() * sizeof(AligendVertex),
                                   MTL::ResourceStorageModeShared);
    m_indices = device->newBuffer(geometry.indices.data(),
                                  geometry.indices.size() * sizeof(uint32_t),
                                  MTL::ResourceStorageModeShared);
    m_num_indices = geometry.indices.size();
}

MTL::AccelerationStructureGeometryDescriptor*
MetalGeometry::getDescriptor() {
    MTL::AccelerationStructureTriangleGeometryDescriptor* descriptor = MTL::AccelerationStructureTriangleGeometryDescriptor::descriptor();

    descriptor->setIndexBuffer(m_indices);
    descriptor->setIndexType(MTL::IndexTypeUInt32);

    descriptor->setVertexBuffer(m_vertices);
    descriptor->setVertexStride(sizeof(AligendVertex));
    descriptor->setTriangleCount(m_num_indices / 3);

    // descriptor->setPrimitiveDataBuffer(m_vertices);
    // descriptor->setPrimitiveDataStride(3 * sizeof(AligendVertex));
    // descriptor->setPrimitiveDataElementSize(3 * sizeof(AligendVertex));

    return descriptor;
}

MetalObject::MetalObject(MTL::Device* device, const Object& object) {
    for (const auto& geometry : object.geometries) {
        MetalGeometry metal_geometry(device, geometry);
        m_geometries.push_back(metal_geometry);
        m_descriptors.push_back(metal_geometry.getDescriptor());
    }
    m_descriptors_array = NS::Array::array((NS::Object**)m_descriptors.data(), m_descriptors.size());
}

}
