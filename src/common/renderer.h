#pragma once

#include <string>
#include <memory>

#include "scene.h"
#include "window.h"

namespace fart {

struct Renderer {
    public:
        virtual ~Renderer() = default;

        virtual void init(Scene &scene, Window &window) = 0;
        virtual void render() = 0;
        virtual std::string name() = 0;
};

}
