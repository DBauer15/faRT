#include "tlas.h"

#include "common/defs.h"
#include <cstdint>
#include <chrono>

namespace fart {

TLAS::TLAS(const std::vector<ObjectInstance>& instances, const std::vector<BVH>& bvhs) {
    m_instances = instances;
    m_bvhs = bvhs;

    build();
}

void
TLAS::build() {
    WARN("Building TLAS");
    auto t_start = std::chrono::high_resolution_clock::now();

    if (m_bvhs.size() == 0 || m_instances.size() == 0) {
        m_tlas_nodes.resize( 1 );
        m_tlas_nodes[0].left_right = 0;
        return;
    }

    size_t N = m_instances.size();
    m_nodes_used = 1;
    m_tlas_nodes.resize( N * 2 );

    // Initialize leaf nodes
    std::vector<uint32_t> bvh_node_offsets;
    bvh_node_offsets.reserve(m_bvhs.size());
    bvh_node_offsets[0] = 0;
    for (size_t i = 1; i < m_bvhs.size(); i++)
        bvh_node_offsets[i] = bvh_node_offsets[i-1] + m_bvhs[i-1].getNodesUsed();

    std::vector<int32_t> node_idx;
    node_idx.resize(N);

    for (uint32_t i = 0; i < N; i++) {
        node_idx[i] = m_nodes_used;
        // TODO: Respect the instance's transform here
        m_tlas_nodes[m_nodes_used].aabb = m_bvhs[m_instances[i].object_id].getNodes()[0].aabb;
        m_tlas_nodes[m_nodes_used].blas = bvh_node_offsets[m_instances[i].object_id];

        m_tlas_nodes[m_nodes_used].left_right = 0;
        m_nodes_used += 1;
    }

    // Agglomerative clustering to build the remainder of the tree
    uint32_t node_indices = N;
    int32_t A = 0, B = findBestMatch(node_idx, node_indices, A);
    while (node_indices > 1) {
        int32_t C = findBestMatch(node_idx, node_indices, B);
        if (A == C) {
            int32_t node_idx_a = node_idx[A], node_idx_b = node_idx[B];
            TLASNode& node_a = m_tlas_nodes[node_idx_a];
            TLASNode& node_b = m_tlas_nodes[node_idx_b];
            TLASNode& new_node = m_tlas_nodes[m_nodes_used];
            new_node.left_right = node_idx_a + (node_idx_b << 16);
            new_node.aabb = node_a.aabb.merge(node_b.aabb);
            node_idx[A] = m_nodes_used++;
            node_idx[B] = node_idx[node_indices - 1];
            B = findBestMatch( node_idx, --node_indices, A);
        } else { 
            A = B; 
            B = C;
        }
    }
    m_tlas_nodes[0] = m_tlas_nodes[node_idx[A]];

    auto build_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start);
    SUCC("Built TLAS over " + std::to_string(m_instances.size()) + " instances in " + std::to_string(build_time_ms.count() / 1000.f) + " seconds");
}

int32_t
TLAS::findBestMatch(std::vector<int32_t>& node_idx, int32_t N, int32_t A) {
    float smallest = 1e30f;
    int32_t best_b = -1;

    for (int32_t B = 0; B < N; B++) if (A != B) {
        AABB common_bounds = m_tlas_nodes[node_idx[A]].aabb.merge(m_tlas_nodes[node_idx[B]].aabb);
        if (common_bounds.surfaceArea() < smallest) {
            smallest = common_bounds.surfaceArea();
            best_b = B;
        }
    }

    return best_b;
}

}
