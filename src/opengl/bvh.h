#pragma once

#include "aabb.h"

#include <vector>
#include <stage.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

using namespace stage;

namespace fart {

struct Vertex {
    stage_vec4f position;
    stage_vec4f normal;
    stage_vec2f uv;
    uint32_t material_id;
    uint32_t padding;
};

struct BVHSplitBucket {
    AABB bounds;
    uint32_t count { 0 };
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
        BVH( const Object& object, BVHSplitMethod split_method = BVHSplitMethod::SAH );

        size_t getNodesUsed() { return m_nodes_used; }
        std::vector<BVHNode>& getNodes() { return m_bvh_nodes; }
        std::vector<Vertex>& getVertices() { return m_vertices; }
        std::vector<uint32_t>& getIndices() { return m_indices; }

    private:
        void build();
        void updateNodeBounds( uint32_t node_idx );
        void subdivide( uint32_t node_idx );
        bool splitEqual(uint32_t node_idx, float& split_pos, uint32_t& axis );
        bool splitSAH(uint32_t node_idx, float& split_pos, uint32_t& axis );

        BVHSplitMethod m_split_method;
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;

        std::vector<BVHNode> m_bvh_nodes;
        std::vector<glm::vec3> m_centroids;
        size_t m_nodes_used;

};

}
