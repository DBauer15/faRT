#include "aabb.h"

namespace fart {

AABB
AABB::transform(const glm::mat4& xfm) const {
    AABB result;

    // From: https://github.com/jbikker/bvh_article/blob/d7141934b00c23d2a0523f24525c511fdec0114c/alltogether.cpp#L266
    for (int i = 0; i < 8; i++)
        result.extend( glm::vec3( xfm * glm::vec4(
                                            glm::vec3( i & 1 ? max.x : min.x, 
                                                       i & 2 ? max.y : min.y, 
                                                       i & 4 ? max.z : min.z ),
                                                 1)));
    return result;
}

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
