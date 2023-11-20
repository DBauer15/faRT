#pragma once

#include "window.h"
#include "scene.h"
#include <memory>

namespace fart {

struct App {
    
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        App(std::string scene);
        ~App() = default;

        void run();

    private:
        Window m_window{WIDTH, HEIGHT, "Hello FaRT!"};

        std::unique_ptr<Scene> m_scene {nullptr};
};

}
