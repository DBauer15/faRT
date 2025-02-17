#include "bvh.h"

#include "common/defs.h"
#include <algorithm>
#include <numeric>
#include <string>
#include <chrono>

/* 
 * Code adapted from "How to build a BVH" and "Physically Based Rendering"
 * References:
 * https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
 * https://pbr-book.org/
 *
 */
namespace fart {

BVH::BVH(const stage::Object& object, BVHSplitMethod split_method) {
    size_t index_offset = 0;

    size_t vertices_size = std::accumulate(object.geometries.begin(), object.geometries.end(), 0, [](size_t acc, const stage::Geometry& el) { return acc + el.positions.size(); });
    m_vertices.resize(vertices_size);
    size_t indices_size = std::accumulate(object.geometries.begin(), object.geometries.end(), 0, [](size_t acc, const stage::Geometry& el) { return acc + el.indices.size(); });
    m_indices.reserve(indices_size);


    //m_vertices.insert(m_vertices.end(), geometry.vertices.begin(), geometry.vertices.end());
    std::memcpy(m_vertices.data(), object.data->data(), vertices_size * sizeof(Vertex));
    for (auto& geometry : object.geometries) {
        m_indices.insert(m_indices.end(), geometry.indices.begin(), geometry.indices.end());
        std::transform(m_indices.end() - geometry.indices.size(), m_indices.end(), m_indices.end() - geometry.indices.size(), [&](uint32_t index) {
                return index + index_offset;
        });
        index_offset += geometry.positions.size();
    }
    m_split_method = split_method;

    build();
}

BVH::BVH(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, BVHSplitMethod split_method) {
    m_vertices = vertices;
    m_indices = indices;
    m_split_method = split_method;

    build();
}

void
BVH::build() {
    WARN("Building BVH.. This may take a while.");
    auto t_start = std::chrono::high_resolution_clock::now();
        
    size_t N = m_indices.size() / 3;
    m_nodes_used = 1;
    m_centroids.resize(N);
    m_bvh_nodes.resize( N * 2 );

    for (size_t i = 0; i < N; ++i) {
        
        const glm::vec3& t1 = m_vertices[m_indices[3*i]].position;
        const glm::vec3& t2 = m_vertices[m_indices[3*i+1]].position;
        const glm::vec3& t3 = m_vertices[m_indices[3*i+2]].position;

        m_centroids[i] = (t1 + t2 + t3) / 3.f;
    }

    uint32_t root_idx = 0;
    BVHNode& root = m_bvh_nodes[root_idx];
    root.left_child = 0;
    root.first_tri_index_id = 0, root.tri_count = N;

    updateNodeBounds( root_idx );
    subdivide( root_idx );

    auto build_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_start);

    LOG("Built BVH over " + std::to_string(N) + " triangles in " + std::to_string(build_time_ms.count() / 1000.f) + " seconds");
}

void
BVH::updateNodeBounds( uint32_t node_idx ) {
    BVHNode& node = m_bvh_nodes[node_idx];
    node.aabb.min = glm::vec3(1e30f);
    node.aabb.max = glm::vec3(-1e30f);

    for (uint32_t i = node.first_tri_index_id; i < node.first_tri_index_id + 3 * node.tri_count; i++) {
        const glm::vec3& t1 = m_vertices[m_indices[i]].position;
        node.aabb.extend(t1);
    }
}

void
BVH::subdivide( uint32_t node_idx ) {
    uint32_t axis = 0;
    float split_pos = 0.f;
    switch (m_split_method) {
        case BVHSplitMethod::Equal: 
            if (!splitEqual(node_idx, split_pos, axis)) return;
            break;
        case BVHSplitMethod::SAH:
            if (!splitSAH(node_idx, split_pos, axis)) return;
            break;
    }

    BVHNode& node = m_bvh_nodes[node_idx];
    int32_t i = node.first_tri_index_id;
    int32_t j = i + (node.tri_count - 1) * 3;
    while (i <= j) {

        if (m_centroids[i/3][axis] < split_pos) {
            i+=3;
        } else {
            std::swap(m_indices[i], m_indices[j]);
            std::swap(m_indices[i+1], m_indices[j+1]);
            std::swap(m_indices[i+2], m_indices[j+2]);
            std::swap(m_centroids[i/3], m_centroids[j/3]);
            j-=3;
        }
    }

    uint32_t left_count = (i - node.first_tri_index_id) / 3;
    if (left_count == 0 || left_count == node.tri_count) return;

    uint32_t left_child_idx = m_nodes_used++;
    uint32_t right_child_idx = m_nodes_used++;

    node.left_child = left_child_idx;

    m_bvh_nodes[left_child_idx].first_tri_index_id = node.first_tri_index_id;
    m_bvh_nodes[left_child_idx].tri_count = left_count;
    m_bvh_nodes[right_child_idx].first_tri_index_id = i;
    m_bvh_nodes[right_child_idx].tri_count = node.tri_count - left_count;

    node.tri_count = 0;

    updateNodeBounds( left_child_idx );
    updateNodeBounds( right_child_idx );

    subdivide( left_child_idx );
    subdivide( right_child_idx );
}

bool
BVH::splitEqual(uint32_t node_idx, float& split_pos, uint32_t& axis) {
    BVHNode& node = m_bvh_nodes[node_idx];
    glm::vec3 extent = node.aabb.extent();
    axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    split_pos = node.aabb.min[axis] + extent[axis] * 0.5f;

    return node.tri_count > 2;
}

/*
 * Reference:
 * https://www.pbr-book.org/4ed/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies
 *
 */
bool
BVH::splitSAH(uint32_t node_idx, float& split_pos, uint32_t& axis) {

    BVHNode& node = m_bvh_nodes[node_idx];
    const glm::vec3 node_extent = node.aabb.extent();
    if (node.tri_count <= 4) return false;

    const int n_buckets = 12;
    const int n_splits = n_buckets - 1;

    int min_cost_split_bucket = -1;
    uint32_t min_split_axis = 0;
    float min_cost = std::numeric_limits<float>::infinity();

    for (size_t ax = 0; ax < 3; ax++) {
        if (node_extent[ax] <= 0.f) continue;
        std::vector<BVHSplitBucket> buckets (n_buckets);

        for (uint32_t i = node.first_tri_index_id; i < node.first_tri_index_id + 3 * node.tri_count; i+=3) {
            int b = n_buckets * std::clamp(((m_centroids[i / 3][ax] - node.aabb.min[ax]) / node_extent[ax]), 0.f, 1.f);
            if (b == n_buckets) b = n_buckets - 1;
            buckets[b].count += 1;
            buckets[b].bounds.extend(m_vertices[m_indices[i]].position);
            buckets[b].bounds.extend(m_vertices[m_indices[i+1]].position);
            buckets[b].bounds.extend(m_vertices[m_indices[i+2]].position);
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

    float leaf_cost = node.tri_count;
    min_cost = 1.f / 2.f + min_cost / node.aabb.surfaceArea();

    axis = min_split_axis;
    split_pos = node.aabb.min[axis] + (node_extent[axis] / n_buckets) * (min_cost_split_bucket + 1);
    
    return node.tri_count > 16 || min_cost < leaf_cost;
}
}
