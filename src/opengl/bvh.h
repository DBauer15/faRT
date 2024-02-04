#pragma once

#include "common/mesh.h"
#include "aabb.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace fart {

struct BVHSplitBucket {
    int count = 0;
    AABB bounds;
};

struct BVHNode {
    AABB aabb;
    uint32_t left_child { 0 };
    uint32_t first_tri_index_id, tri_count, filler;
};

enum BVHSplitMethod {
    Equal,
    SAH,
};

struct BVH {

    public:
        BVH( const std::vector<Geometry>& geometries, BVHSplitMethod split_method = BVHSplitMethod::SAH );
        BVH( std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices, BVHSplitMethod split_method = BVHSplitMethod::SAH );

        size_t getNodesUsed() { return m_nodes_used; }
        std::vector<BVHNode>& getNodes() { return m_bvh_nodes; }
        std::vector<AligendVertex>& getVertices() { return m_vertices; }
        std::vector<uint32_t>& getIndices() { return m_indices; }

    private:
        void build();
        void updateNodeBounds( uint32_t node_idx );
        void subdivide( uint32_t node_idx );
        bool splitEqual(uint32_t node_idx, float& split_pos, uint32_t& axis );
        bool splitSAH(uint32_t node_idx, float& split_pos, uint32_t& axis );

        BVHSplitMethod m_split_method;
        std::vector<AligendVertex> m_vertices;
        std::vector<uint32_t> m_indices;

        std::vector<BVHNode> m_bvh_nodes;
        std::vector<glm::vec3> m_centroids;
        size_t m_nodes_used;

};

}
