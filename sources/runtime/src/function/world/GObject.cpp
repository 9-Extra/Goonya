#include "GObject.h"
#include "function/world/Component.h"
#include "function/world/World.h"


namespace Goonya {

bool GObject::remove_component(const std::type_info &t_info) {
    for (auto it = components.begin(); it != components.end(); ++it) {
        auto &c = **it; // 比较其内容而非智能指针
        if (get_world()) {
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

void GObject::set_world(World *world) noexcept {
    if (world == _world) {
        return;
    } else {
        // exit world
        if (_world) {
            do_unregister();
        }

        _world = world; // 提前设置使component可以获取正确的世界
        // enter world
        if (world) {
            do_register();
        }

        for (auto &child : children) {
            child->set_world(world);
        }
    }
}
void GObject::recaculate_world_transform() noexcept {
    GN_ASSERT(is_world_transform_dirty);
    if (auto p = parent.lock(); p) {
        // 递归计算父节点的世界变换
        if (p->is_world_transform_dirty) {
            p->recaculate_world_transform();
        }
        // 子节点的transform为父节点的transform叠加上自身的transform
        // 从逻辑上是先进行子节点的变换，再进行父节点的变换
        world_model_matrix = transform.model_matrix() * p->world_model_matrix;
        world_normal_matrix = transform.normal_matrix() * p->world_normal_matrix;
    } else {
        // 对于根节点特殊处理
        world_model_matrix = transform.model_matrix();
        world_normal_matrix = transform.normal_matrix();
    }
    is_world_transform_dirty = false;
}

void GObject::do_register() {
    GN_ASSERT(!_is_registered);
    _is_registered = true;

    for (auto &&component : components) {
        component->on_register();
    }
    // queue_deferred_update(ComponentUpdateFlag::INITALIZE); 刚注册时不需要更新
}
void GObject::do_unregister() {
    GN_ASSERT(_is_registered);
    _is_registered = false;
    for (auto &&component : components) {
        component->on_unregister();
    }

    // 如果已经注册到了更新队列里，则取出来
    if (cpnt_update_flag != ComponentUpdateFlag::NONE) {
        get_world()->remove_deferred_update(this->weak_from_this());
        cpnt_update_flag = ComponentUpdateFlag::NONE;
    }
}

void GObject::do_deferred_update() {
    GN_ASSERT(_is_registered);
    for (auto &&component : components) {
        component->on_update(cpnt_update_flag);
    }
    cpnt_update_flag = ComponentUpdateFlag::NONE; // deferred_update_list由World来清除
}

void GObject::queue_deferred_update(ComponentUpdateFlag flag) noexcept {
    GN_ASSERT(flag != ComponentUpdateFlag::NONE);
    if (!_is_registered) {
        return; // 不在世界中的组件不会收到更新
    }
    // 如果当前的cpnt_update_flag为NONE，说明还没有注册到deferred_update_list
    if (cpnt_update_flag == ComponentUpdateFlag::NONE) {
        get_world()->add_deferred_update(weak_from_this());
    }
    cpnt_update_flag |= flag;

    // 不会递归更新子节点
}

} // namespace Goonya
