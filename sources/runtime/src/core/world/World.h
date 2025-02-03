#pragma once

#include "GObject.h"


namespace Goonya {
class World {
public:
    std::shared_ptr<GObject> root;
    
    World() {
        reset();
    }

    void reset() {
        root = nullptr;
        tick_count = 0;
    }

    uint64_t get_tick_count() { return tick_count; }

    void tick();

private:
    uint64_t tick_count;
};

extern World world;
}