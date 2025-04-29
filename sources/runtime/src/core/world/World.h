#pragma once

#include "GObject.h"

namespace Goonya {
class World {
private:
    uint64_t tick_count = 0;
public:
    std::shared_ptr<GObject> root;

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