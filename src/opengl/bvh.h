#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "common/mesh.h"

namespace fart {

struct BVHNode {
    public:
        glm::vec4 aabb_min, aabb_max;
        uint32_t left_child;

        uint32_t first_tri_index_id, tri_count, filler;
};

struct BVH {

    public:
        BVH(std::vector<Geometry>& geometries);
        BVH(std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        std::vector<BVHNode>& getNodes() { return m_bvh_nodes; }
        std::vector<AligendVertex>& getVertices() { return m_vertices; }
        std::vector<uint32_t>& getIndices() { return m_indices; }

    private:
        void build( std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        void updateNodeBounds( uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        void subdivide( uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);

        std::vector<AligendVertex> m_vertices;
        std::vector<uint32_t> m_indices;

        std::vector<BVHNode> m_bvh_nodes;
        std::vector<glm::vec3> m_centroids;
        size_t m_nodes_used;

};

}
