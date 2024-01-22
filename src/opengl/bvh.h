#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "common/mesh.h"

namespace fart {

struct Bounds {
    alignas(16) glm::vec3 min { 1e30f };
    alignas(16) glm::vec3 max { -1e30f };

    Bounds merge(const Bounds& other) {
        return {
            { std::min(min.x, other.min.x), std::min(min.y, other.min.y), std::min(min.z, other.min.z) },
            { std::max(max.x, other.max.x), std::max(max.y, other.max.y), std::max(max.z, other.max.z) }
        };
    }

    void extend(const glm::vec3& other) {
        min.x = std::min(min.x, other.x);
        min.y = std::min(min.y, other.y);
        min.z = std::min(min.z, other.z);
        max.x = std::max(max.x, other.x);
        max.y = std::max(max.y, other.y);
        max.z = std::max(max.z, other.z);
    }

    glm::vec3 extent() const {
        return max - min;
    }

    float surfaceArea() const {
        glm::vec3 d = extent();
        if (d.x < 0.f || d.y < 0.f || d.z < 0.f) return 0.f;
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }
};

struct BVHNode {
    public:
        Bounds aabb;
        uint32_t left_child { 0 };
        uint32_t first_tri_index_id, tri_count, filler;
};

struct BVHSplitBucket {
    int count = 0;
    Bounds bounds;
};

struct BVH {

    public:
        BVH(std::vector<Geometry>& geometries);
        BVH(std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        size_t getNodesUsed() { return m_nodes_used; }
        std::vector<BVHNode>& getNodes() { return m_bvh_nodes; }
        std::vector<AligendVertex>& getVertices() { return m_vertices; }
        std::vector<uint32_t>& getIndices() { return m_indices; }

    private:
        void build( std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        void updateNodeBounds( uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        void subdivide( uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices);
        bool splitCentroid( uint32_t axis, uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices, float& split_pos);
        bool splitSAH( uint32_t axis, uint32_t node_idx, std::vector<AligendVertex>& vertices, std::vector<uint32_t>& indices, float& split_pos);

        std::vector<AligendVertex> m_vertices;
        std::vector<uint32_t> m_indices;

        std::vector<BVHNode> m_bvh_nodes;
        std::vector<glm::vec3> m_centroids;
        size_t m_nodes_used;

};

}
