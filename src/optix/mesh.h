#pragma once

#include <stage.h>
#include "buffer.h"

using namespace stage;

namespace fart {

struct OptixGeometrySBTData {
    Geometry* vertices;
    uint32_t* indices;
};

struct OptixGeometry {
    public:
        OptixGeometry(const Geometry& geometry);

        CudaBuffer& getVertices() { return m_vertices; }
        CudaBuffer& getIndices() { return m_indices; }
        OptixGeometrySBTData getSBTData();

    private:
        CudaBuffer m_vertices;
        CudaBuffer m_indices;
};

struct OptixObject {
    public:
        OptixObject(const Object& object);

        std::vector<OptixGeometry>& getGeometries() { return m_geometries; }

    private:
        std::vector<OptixGeometry> m_geometries;
};

struct OptixObjectInstance {

};

}
