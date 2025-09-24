#include "World.h"
#include "core/ThreadPool.h"

namespace Goonya {
World world; // global world instance

void World::tick() {
    tick_count++;

    if (this->root){
        this->root->set_world(true);
        this->root->tick(GObject::DirtyFlag::DEFAULT);
    }

    main_thread_process();
}
}