#include "World.h"

namespace Goonya {
World world; // global world instance

void World::tick() {
    tick_count++;

    // 调用所有的系统
    for (const auto &[name, sys] : systems) {
        if (sys->enable) {
            sys->tick();
        }
    }
    // 递归更新所有物体
    this->root->tick(0);

    //上传天空盒
    //renderer.set_skybox();
}
}