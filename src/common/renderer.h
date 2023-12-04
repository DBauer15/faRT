#pragma once

#include <string>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "defs.h"
#include "scene.h"
#include "window.h"

namespace fart {

struct Renderer {
    public:
        virtual ~Renderer() = default;

        virtual void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) = 0;
        virtual void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up) = 0;
        virtual std::string name() = 0;
};

}
