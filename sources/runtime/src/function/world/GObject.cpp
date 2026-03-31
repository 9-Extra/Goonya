#include "GObject.h"
#include "function/world/Component.h"
#include "function/world/World.h"

namespace Goonya {

std::shared_ptr<GObject> GObject::clone(bool recursive) const {
    // 创建新对象，拷贝基本属性
    auto new_obj = std::make_shared<GObject>(this->name);
    new_obj->transform = this->transform;
    new_obj->world_model_matrix = this->world_model_matrix;
    new_obj->is_world_transform_dirty = this->is_world_transform_dirty;
    new_obj->is_enabled = this->is_enabled;

    // 特殊处理的成员：新对象不属于任何世界，未注册，没有父物体，不需要更新
    new_obj->_world = nullptr;
    new_obj->_is_registered = false;
    new_obj->parent.reset();
    new_obj->cpnt_update_flag = ComponentUpdateFlag::NONE;

    // 深拷贝所有组件（使用 add_component 确保一致性，由于 _world 为 nullptr，不会触发 on_register）
    for (const auto &comp : this->components) {
        new_obj->add_component(comp->clone());
    }

    // 递归拷贝子物体
    if (recursive) {
        for (const auto &child : this->children) {
            auto cloned_child = child->clone(true);
            new_obj->attach_child(cloned_child);
        }
    }

    return new_obj;
}
std::shared_ptr<GObject> GObject::get_child_by_path(std::string_view path) noexcept {
    // 空路径返回自身
    if (path.empty()) {
        return shared_from_this();
    }

    std::shared_ptr<GObject> current = shared_from_this();

    // 按/分割路径并逐级查找
    size_t start = 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string_view::npos) {
            end = path.size();
        }

        std::string_view name = path.substr(start, end - start);
        // 查找当前名称的子节点
        std::shared_ptr<GObject> next = nullptr;
        for (auto &child : current->children) {
            if (child->name == name) {
                next = child;
                break;
            }
        }

        if (!next) {
            return nullptr;
        }

        current = next;
        start = end + 1;
    }

    return current;
}
std::string GObject::generate_unique_name() const noexcept {
    // 检查 "obj" 是否可用
    if (!has_child_with_name("obj")) {
        return "obj";
    }
    // 尝试 "obj1", "obj2", ...
    for (int index = 1; index < 1024; index++) {
        std::string candidate = std::format("obj{}", index);
        if (!has_child_with_name(candidate)) {
            return candidate;
        }
    }

    return "obj_too_many";
}

// ----------------------------------------------------------

void GObject::add_component(std::unique_ptr<Component> &&component) {
    component->set_owner(this);
    if (get_world()) {
        component->on_register();
    }
    components.push_back(std::move(component));
}
Component *GObject::get_component(const std::type_info &t_info) noexcept {
    for (auto &component : components) {
        auto &c = *component; // 比较其内容而非智能指针
        if (typeid(c) == t_info) {
            return component.get();
        }
    }
    return nullptr;
}

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
        if (_world && _is_registered) {
            do_unregister();
        }

        _world = world; // 提前设置使component可以获取正确的世界
        // enter world
        if (world && is_enabled) {
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
    } else {
        // 对于根节点特殊处理
        world_model_matrix = transform.model_matrix();
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
