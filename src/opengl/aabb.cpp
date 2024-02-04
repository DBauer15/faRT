#include "aabb.h"

namespace fart {

AABB
AABB::merge(const AABB& other) const {
    // Inline min/max faster than glm and std equivalents
    return {
        {   
            min.x > other.min.x ? other.min.x : min.x, 
                min.y > other.min.y ? other.min.y : min.y, 
                min.z > other.min.z ? other.min.z : min.z, 
        },
            {   
                max.x < other.max.x ? other.max.x : max.x,
                max.y < other.max.y ? other.max.y : max.y,
                max.z < other.max.z ? other.max.z : max.z,
            }
    };
}

void
AABB::extend(const glm::vec3& other) {
    // Inline min/max faster than glm and std equivalents
    min.x = min.x > other.x ? other.x : min.x;
    min.y = min.y > other.y ? other.y : min.y;
    min.z = min.z > other.z ? other.z : min.z;
    max.x = max.x < other.x ? other.x : max.x;
    max.y = max.y < other.y ? other.y : max.y;
    max.z = max.z < other.z ? other.z : max.z;
}

}
