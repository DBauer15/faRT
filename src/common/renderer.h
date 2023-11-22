#pragma once

#include <string>
#include <memory>

#include "scene.h"
#include "window.h"

namespace fart {

struct Renderer {
    public:
        void init(const Scene &scene);
        void render(const Window &window);

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl;
};

}
