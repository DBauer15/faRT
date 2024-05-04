#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <stage.h>

#include "aabb.h"
#include "bvh.h"

using namespace stage;

namespace fart {

struct TLASSplitBucket {
    AABB bounds;
    uint32_t count { 0 };
};

struct TLASNode {
    AABB aabb;
    uint32_t left_child { 0 };
    uint32_t first_instance_id, instance_count;
    uint32_t filler;
};

struct TLAS {
public:
    TLAS(const std::vector<ObjectInstance>& instances, const std::vector<BVH>& bvhs);

    size_t getNodesUsed() { return m_nodes_used; }
    std::vector<TLASNode>& getNodes() { return m_tlas_nodes; }
    std::vector<ObjectInstance>& getInstances() { return m_instances; }
    std::vector<uint32_t>& getBLASOffsets() { return m_bvh_node_offsets; }

    private:
        void build();
        void updateNodeBounds(uint32_t node_idx);
        void subdivide(uint32_t node_idx);
        bool splitSAH(uint32_t node_idx, float& split_pos, uint32_t& axis);

        std::vector<ObjectInstance> m_instances;
        std::vector<BVH> m_bvhs;

        std::vector<AABB> m_bounds;
        std::vector<uint32_t> m_bvh_node_offsets;
        std::vector<TLASNode> m_tlas_nodes;
        size_t m_nodes_used;
};

}
