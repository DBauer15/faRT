#pragma once

#include <string>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "defs.h"
#include "scene.h"
#include "window.h"

namespace fart {

struct RenderStats {
    float frame_time_ms;
};

struct Renderer {
    public:
        virtual ~Renderer() = default;

        virtual void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) = 0;
        virtual void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) = 0;
        virtual std::string name() = 0;
};

}
