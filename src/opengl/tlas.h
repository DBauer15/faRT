#pragma once

#include "common/mesh.h"
#include "aabb.h"
#include "bvh.h"


namespace fart {

struct TLASNode {
    AABB aabb;
    uint32_t blas;
    uint32_t instance;
    uint32_t left { 0 };
    uint32_t right { 0 };
};

struct TLAS {
    public:
        TLAS(const std::vector<ObjectInstance>& instances, const std::vector<BVH>& bvhs);

        size_t getNodesUsed() { return m_nodes_used; }
        std::vector<TLASNode>& getNodes() { return m_tlas_nodes; }

    private:
        void build();
        int32_t findBestMatch(std::vector<int32_t>& node_idx, int32_t N, int32_t A);


        std::vector<ObjectInstance> m_instances;
        std::vector<BVH> m_bvhs;

        std::vector<TLASNode> m_tlas_nodes;
        size_t m_nodes_used;
};

}
