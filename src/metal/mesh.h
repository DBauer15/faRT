#pragma once

#include <vector>
#include <Metal/Metal.hpp>

#include <stage.h>

using namespace stage;

namespace fart {

struct MetalGeometry {
    public:
        MetalGeometry(MTL::Device* device, const Geometry& geometry);

        MTL::AccelerationStructureGeometryDescriptor* getDescriptor();
        std::vector<MTL::Resource*> getResources() {
            return { m_vertices, m_indices };
        }

    private:
        uint32_t m_num_indices;
        MTL::Buffer* m_vertices;
        MTL::Buffer* m_indices;


};

struct MetalObject {
    public:
        MetalObject(MTL::Device* device, const Object& object);

        std::vector<MetalGeometry>& getGeometries() { return m_geometries; }
        std::vector<MTL::AccelerationStructureGeometryDescriptor*> getGeometryDescriptors() { return m_descriptors; }
        NS::Array* getGeometryDescriptorsArray() { return m_descriptors_array; }

    private:
        std::vector<MetalGeometry> m_geometries;
        std::vector<MTL::AccelerationStructureGeometryDescriptor*> m_descriptors;
        NS::Array* m_descriptors_array;
};

}
