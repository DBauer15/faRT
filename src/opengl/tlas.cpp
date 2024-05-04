#include "tlas.h"

#include "common/defs.h"
#include <string>
#include <cstdint>
#include <chrono>
#include <algorithm>

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

    size_t N = m_instances.size();
    m_nodes_used = 1;
    m_bounds.resize(N);
    m_tlas_nodes.resize(N * 2);
    m_bvh_node_offsets.resize(m_bvhs.size());
    m_bvh_node_offsets[0] = 0;

    for (size_t i = 0; i < N; ++i) {
        m_bounds[i] = m_bvhs[m_instances[i].object_id].getNodes()[0].aabb.transform(glm::make_mat4(m_instances[i].instance_to_world.v));
    }
    for (size_t i = 1; i < m_bvhs.size(); i++) {
        m_bvh_node_offsets[i] = m_bvh_node_offsets[i - 1] + m_bvhs[i - 1].getNodesUsed();
    }

    uint32_t root_idx = 0;
    TLASNode& root = m_tlas_nodes[root_idx];
    root.left_child = 0;
    root.first_instance_id = 0;
    root.instance_count = N;
    updateNodeBounds(root_idx);
    subdivide(root_idx);

    auto build_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start);
    SUCC("Built TLAS over " + std::to_string(m_instances.size()) + " instances in " + std::to_string(build_time_ms.count() / 1000.f) + " seconds");
}

void
TLAS::updateNodeBounds(uint32_t node_idx) {
    TLASNode& node = m_tlas_nodes[node_idx];
    node.aabb.min = glm::vec3(1e30f);
    node.aabb.max = glm::vec3(-1e30f);

    for (uint32_t i = node.first_instance_id; i < node.first_instance_id + node.instance_count; i++) {
        node.aabb = node.aabb.merge(m_bounds[i]);
    }
}

void
TLAS::subdivide(uint32_t node_idx) {
    uint32_t axis = 0;
    float split_pos = 0.f;
    if (!splitSAH(node_idx, split_pos, axis))
        return;

    TLASNode& node = m_tlas_nodes[node_idx];
    int32_t i = node.first_instance_id;
    int32_t j = i + node.instance_count - 1;
    while (i <= j) {
        if (m_bounds[i].centroid()[axis] < split_pos) {
            i += 1;
        }
        else {
            std::swap(m_instances[i], m_instances[j]);
            std::swap(m_bounds[i], m_bounds[j]);
            j -= 1;
        }
    }

    uint32_t left_count = i - node.first_instance_id;
    if (left_count == 0 || left_count == node.instance_count) return;

    uint32_t left_child_idx = m_nodes_used++;
    uint32_t right_child_idx = m_nodes_used++;
    
    node.left_child = left_child_idx;

    m_tlas_nodes[left_child_idx].first_instance_id = node.first_instance_id;
    m_tlas_nodes[left_child_idx].instance_count = left_count;
    m_tlas_nodes[right_child_idx].first_instance_id = i;
    m_tlas_nodes[right_child_idx].instance_count = node.instance_count - left_count;

    node.instance_count = 0;

    updateNodeBounds(left_child_idx);
    updateNodeBounds(right_child_idx);

    subdivide(left_child_idx);
    subdivide(right_child_idx);

}

bool
TLAS::splitSAH(uint32_t node_idx, float& split_pos, uint32_t& axis){
    TLASNode& node = m_tlas_nodes[node_idx];
    const glm::vec3 node_extent = node.aabb.extent();
    if (node.instance_count <= 4) return false;

    const int n_buckets = 12;
    const int n_splits = n_buckets - 1;

    int min_cost_split_bucket = -1;
    uint32_t min_split_axis = 0;
    float min_cost = std::numeric_limits<float>::infinity();

    for (size_t ax = 0; ax < 3; ax++) {
        if (node_extent[ax] <= 0.f) continue;
        std::vector<TLASSplitBucket> buckets (n_buckets);

        for (uint32_t i = node.first_instance_id; i < node.first_instance_id + node.instance_count; i++) {
            int b = n_buckets * std::clamp(((m_bounds[i].centroid()[ax] - node.aabb.min[ax]) / node_extent[ax]), 0.f, 1.f);
            if (b == n_buckets) b = n_buckets - 1;
            buckets[b].count += 1;
            buckets[b].bounds = buckets[b].bounds.merge(m_bounds[i]);
        }

        std::vector<float> costs (n_splits);
        int count_below = 0;
        AABB bound_below;
        for (int i = 0; i < n_splits; i++) {
            bound_below = bound_below.merge(buckets[i].bounds);
            count_below += buckets[i].count;
            costs[i] += count_below * bound_below.surfaceArea();
        }

        int count_above = 0;
        AABB bound_above;
        for (int i = n_splits; i >= 1; i--) {
            bound_above = bound_above.merge(buckets[i].bounds);
            count_above += buckets[i].count;
            costs[i - 1] += count_above * bound_above.surfaceArea();
        }

        for (int i = 0; i < n_splits; i++) {
            if (costs[i] < min_cost) {
                min_cost = costs[i];
                min_cost_split_bucket = i;
                min_split_axis = ax;
            }
        }

    }
    if (min_cost_split_bucket < 0) return false;

    float leaf_cost = node.instance_count;
    min_cost = 1.f / 2.f + min_cost / node.aabb.surfaceArea();

    axis = min_split_axis;
    split_pos = node.aabb.min[axis] + (node_extent[axis] / n_buckets) * (min_cost_split_bucket + 1);
    
    return node.instance_count > 16 || min_cost < leaf_cost;
}

}
