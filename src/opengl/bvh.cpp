#include "bvh.h"

#include "common/defs.h"
#include <algorithm>
#include <string>

namespace fart {

BVH::BVH(std::vector<Geometry>& geometries) {
    size_t index_offset = 0;
    for (auto& geometry : geometries) {
        m_vertices.reserve(m_vertices.size() + geometry.vertices.size());
        m_vertices.insert(m_vertices.end(), geometry.vertices.begin(), geometry.vertices.end());
        m_indices.reserve(m_indices.size() + geometry.indices.size());
        m_indices.insert(m_indices.end(), geometry.indices.begin(), geometry.indices.end());

        std::transform(m_indices.end() - geometry.indices.size(), m_indices.end(), m_indices.end() - geometry.indices.size(), [&](uint32_t index) {
                return index + index_offset;
        });
        index_offset += geometry.vertices.size();
    }

    build(m_vertices, m_indices);
}

BVH::BVH(std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices) {
    m_vertices = vertices;
    m_indices = indices;
    build(m_vertices, m_indices);
}

void
BVH::build(std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices) {
        
    size_t N = indices.size() / 3;
    m_nodes_used = 1;
    m_centroids.resize(N);
    m_bvh_nodes.resize( N * 2 - 1 );

    for (size_t i = 0; i < N; ++i) {
        
        const glm::vec3& t1 = vertices[indices[3*i]].position; //glm::make_vec3(&vertices[3*indices[3*i]]);
        const glm::vec3& t2 = vertices[indices[3*i+1]].position; //glm::make_vec3(&vertices[3*indices[3*i+1]]);
        const glm::vec3& t3 = vertices[indices[3*i+2]].position; //glm::make_vec3(&vertices[3*indices[3*i+2]]);

        m_centroids[i] = (t1 + t2 + t3) / 3.f;
    }

    uint32_t root_idx = 0;
    BVHNode& root = m_bvh_nodes[root_idx];
    root.left_child = 0;
    root.first_tri_index_id = 0, root.tri_count = N;

    updateNodeBounds( root_idx, vertices, indices );
    subdivide( root_idx, vertices, indices );

    SUCC("Built BVH over " + std::to_string(N) + " triangles");
}

void
BVH::updateNodeBounds( uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices) {
    BVHNode& node = m_bvh_nodes[node_idx];
    node.aabb_min = glm::vec4(1e30f);
    node.aabb_max = glm::vec4(-1e30f);

    for (uint32_t i = node.first_tri_index_id; i < node.first_tri_index_id + 3 * node.tri_count; i++) {
        const glm::vec3& t1 = vertices[indices[i]].position;

        node.aabb_min.x = std::min(node.aabb_min.x, t1.x);
        node.aabb_min.y = std::min(node.aabb_min.y, t1.y);
        node.aabb_min.z = std::min(node.aabb_min.z, t1.z);
        node.aabb_max.x = std::max(node.aabb_max.x, t1.x);
        node.aabb_max.y = std::max(node.aabb_max.y, t1.y);
        node.aabb_max.z = std::max(node.aabb_max.z, t1.z);
    }
}

void
BVH::subdivide( uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices) {

    BVHNode& node = m_bvh_nodes[node_idx];
    if (node.tri_count <= 2) return;

    //WARN("Processing BVH node " + std::to_string(node_idx));
    glm::vec4 extent = node.aabb_max - node.aabb_min;

    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    float split_pos = node.aabb_min[axis] + extent[axis] * 0.5f;

    uint32_t i = node.first_tri_index_id;
    uint32_t j = i + (node.tri_count - 1)*3;
    while (i <= j) {

        if (m_centroids[i/3][axis] < split_pos) {
            i+=3;
        } else {
            std::swap(indices[i], indices[j]);
            std::swap(indices[i+1], indices[j+1]);
            std::swap(indices[i+2], indices[j+2]);
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

    updateNodeBounds( left_child_idx, vertices, indices );
    updateNodeBounds( right_child_idx, vertices, indices );

    subdivide( left_child_idx, vertices, indices );
    subdivide( right_child_idx, vertices, indices );
}

}
