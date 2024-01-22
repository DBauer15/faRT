#pragma once

#include "common/mesh.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace fart {

struct Bounds {
    alignas(16) glm::vec3 min { 1e30f };
    alignas(16) glm::vec3 max { -1e30f };

    Bounds merge(const Bounds& other) const;

    void extend(const glm::vec3& other);

    glm::vec3 extent() const {
        return { max.x - min.x, max.y - min.y, max.z - min.z };
    }

    float surfaceArea() const {
        glm::vec3 d = extent();
        if (d.x < 0.f || d.y < 0.f || d.z < 0.f) return 0.f;
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }
};

struct BVHSplitBucket {
    int count = 0;
    Bounds bounds;
};

struct BVHNode {
    Bounds aabb;
    uint32_t left_child { 0 };
    uint32_t first_tri_index_id, tri_count, filler;
};

enum BVHSplitMethod {
    Equal,
    SAH,
};

struct BVH {

    public:
        BVH( std::vector<Geometry>& geometries, BVHSplitMethod split_method = BVHSplitMethod::SAH );
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
