#include "World.h"

namespace Goonya {
World world; // global world instance

void World::tick() {
    tick_count++;

    if (this->root){
        this->root->set_world(true);
        this->root->tick(GObject::DirtyFlag::DEFAULT);
    }

    // 递归更新所有物体
}
}