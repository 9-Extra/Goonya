#include "World.h"
#include "core/ThreadPool.h"
#include <cassert>

namespace Goonya {

std::forward_list<World> World::world_list;

void World::tick() {
    tick_count++;

    assert(root && root->get_world() == this);
    
    for(const auto& t: tick_functions){
        t->tick();
    }
    
    for(auto&& obj: deferred_update_list){
        if (auto p = obj.lock();p){
            p->do_deferred_update();
        }
    }
    deferred_update_list.clear();

    main_thread_process();
}
}