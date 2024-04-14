#include "World.h"

namespace Goonya {
World world; // global world instance

void World::tick() {
    tick_count++;

    // 递归更新所有物体
    this->root->tick(GObject::DirtyFlag::DEFAULT);

    //上传天空盒
    //renderer.set_skybox();
}
}