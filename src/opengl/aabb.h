#pragma once

#include <glm/glm.hpp>

namespace fart {

struct AABB {
    alignas(16) glm::vec3 min { 1e30f };
    alignas(16) glm::vec3 max { -1e30f };

    AABB transform(const glm::mat4& xfm) const;
    AABB merge(const AABB& other) const;

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

}
