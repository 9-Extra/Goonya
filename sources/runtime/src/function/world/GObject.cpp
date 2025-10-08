#include "GObject.h"

namespace Goonya {
void GObject::tick(DirtyFlag parent_flag) {
    if (is_disabled())
        return; // 跳过不启用的物体

    dirty_flag.append(parent_flag);

    if (dirty_flag[DirtyFlag::TRANSFORM_DIRTY]) {
        recaculate_world_transform();
    }
    // 更新组件
    // todo：如果在更新组件时组件增删了components怎么办
    for (auto &c : get_components()) {
        c->on_tick();
    }

    for (auto &child : get_children()) {
        child->tick(dirty_flag); // 更新子节点
    }

    dirty_flag.clear(); // 清理标记
}
bool GObject::remove_component(const std::type_info &t_info) {
    for (auto it = components.begin(); it != components.end(); ++it) {
        auto &c = **it; // 比较其内容而非智能指针
        if (_is_in_world) {
            c.on_unregister();
        }
        c.set_owner(nullptr);
        if (typeid(c) == t_info) {
            components.erase(it);
            return true;
        }
    }
    return false;
}

void GObject::set_world(bool is_in_world) noexcept {
    if (_is_in_world == is_in_world) {
        return;
    } else {
        _is_in_world = is_in_world;
        if (is_in_world) {
            // enter world
            recaculate_world_transform();
            if (!is_disabled()) {
                for (auto &component : components) {
                    component->on_register();
                }
            }
        } else {
            // exit world
            if (!is_disabled()) {
                for (auto &component : components) {
                    component->on_unregister();
                }
            }
        }
        for (auto &child : children) {
            child->set_world(is_in_world);
        }
    }
}
} // namespace Goonya
