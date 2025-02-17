#pragma once

#include <glm/glm.hpp>

namespace fart {

struct WebGPURendererUniforms {
    alignas(4) uint32_t frame_number;
    alignas(4) float scene_scale;
    alignas(4) float aspect_ratio;
    alignas(16) glm::vec4 eye;
    alignas(16) glm::vec4 dir;
    alignas(16) glm::vec4 up;
    alignas(16) glm::u32vec4 viewport_size;
};

struct Vertex {
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 normal;
    alignas(16) glm::vec4 uv;
};

}
