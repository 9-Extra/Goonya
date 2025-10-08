#pragma once

#include "GObject.h"
#include "function/renderer/RenderScene.h"
#include "function/renderer/Renderer.h"

namespace Goonya {
class World final {
public:
    std::shared_ptr<GObject> root;

private:
    uint64_t tick_count = 0;
    Graphics::RenderScene *_main_scene = nullptr;

public:
    World() { _main_scene = Graphics::renderer.create_scene(); }
    ~World() {
        Graphics::renderer.drop_scene(_main_scene);
        _main_scene = nullptr;
    }

    Graphics::RenderScene *main_scene() noexcept {
        assert(_main_scene != nullptr);
        return _main_scene;
    }

    void reset() {
        if (root) {
            root->set_world(false);
        }
        root = nullptr;
        tick_count = 0;
    }

    uint64_t get_tick_count() const { return tick_count; }

    void tick();
};

extern World world;
} // namespace Goonya