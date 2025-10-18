#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "Component.h"
#include "core/cgmath.h"
#include "core/enum_operator.h"

namespace Goonya {

class World;

enum class DirtyFlag {
    NONE = 0,
    WORLD_TRANSFORM_DIRTY = 1,
};
DECLARE_ENUM_OPERATORS(DirtyFlag);

// 一个物体，单独的物体没有功能，也不可见，需要挂上组件来实现具体的功能
class GObject final : public std::enable_shared_from_this<GObject> {
private:
    std::string name;
    World *_world = nullptr;

    Transform transform; // 相对父节点的变换

    Matrix4 world_model_matrix;  // 世界根节点的变换
    Matrix3 world_normal_matrix; // 世界根节点的法线变换

    std::vector<std::unique_ptr<Component>> components;
    std::weak_ptr<GObject> parent;
    std::vector<std::shared_ptr<GObject>> children;

    bool is_world_transform_dirty : 1 = true;

    bool disabled : 1 = false;
    bool _is_registered : 1 = false;

    ComponentUpdateFlag cpnt_update_flag = ComponentUpdateFlag::NONE;

public:
    explicit GObject(std::string name = "") noexcept : name(std::move(name)) {};
    explicit GObject(const Transform &transform, std::string name = "") noexcept
        : name(std::move(name)), transform(transform) {};

    ~GObject() {
        if (_is_registered) {
            do_unregister();
        }
        assert(!parent.lock()); // 必须没有父节点
    };

    std::string_view get_name() const noexcept { return name; }
    World *get_world() const noexcept { return _world; }

    void enable() noexcept { // NOLINT: 手动保证没有循环引用
        if (!is_disabled())
            return;
        disabled = false;
        if (get_world()) {
            do_register();
        }
        for (std::shared_ptr<GObject> &child : children) {
            child->enable();
        }
    }

    void disable() noexcept { // NOLINT: 手动保证没有循环引用
        if (is_disabled())
            return;
        disabled = true;
        if (get_world()) {
            do_unregister();
        }
        for (std::shared_ptr<GObject> &child : children) {
            child->disable();
        }
    }

    void add_component(std::unique_ptr<Component> &&component) {
        component->set_owner(this);
        if (get_world()) {
            component->on_register();
        }
        components.push_back(std::move(component));
    }

    Component *get_component(const std::type_info &t_info) noexcept {
        for (auto &component : components) {
            auto &c = *component; // 比较其内容而非智能指针
            if (typeid(c) == t_info) {
                return component.get();
            }
        }
        return nullptr;
    }

    template <class T>
        requires(std::is_base_of_v<Component, T>)
    T *get_component() noexcept {
        return (T *)get_component(typeid(T));
    }

    bool remove_component(const std::type_info &t_info);

    template <class T>
        requires(std::is_base_of_v<Component, T>)
    bool remove_component() {
        return remove_component(typeid(T));
    }

    std::vector<std::unique_ptr<Component>> &get_components() noexcept { return components; }

    void set_transform(const Transform &transform) noexcept {
        this->transform = transform;
        mark_world_transform_dirty();
    }

    const Transform &get_transform() const noexcept { return transform; }

    void translate(Vector3f distance) noexcept { set_position(transform.position + distance); }

    void set_position(Vector3f pos) noexcept {
        this->transform.position = pos;
        mark_world_transform_dirty();
    }

    void rotate_local_axis(Vector3f angle) noexcept { rotate_local_axis(Quaternion::from_eular(angle)); }
    void rotate_local_axis(Quaternion rotation) noexcept { set_rotation(this->transform.rotation * rotation); }

    void rotate_global_axis(Vector3f angle) noexcept { rotate_global_axis(Quaternion::from_eular(angle)); }
    void rotate_global_axis(Quaternion rotation) noexcept {
        // 沿全局坐标系旋转（原点依然是物体中心而非世界中心），依赖于父节点相对世界的旋转
        Quaternion new_rotation;
        if (has_parent()) {
            Quaternion parent_rotation = parent.lock()->get_world_model_matrix().resolve_rotation();
            new_rotation = parent_rotation.conjugate() * rotation * parent_rotation * this->transform.rotation;
        } else {
            new_rotation = rotation * this->transform.rotation;
        }
        set_rotation(new_rotation);
    }

    void set_rotation(Quaternion rotation) noexcept {
        this->transform.rotation = rotation;
        mark_world_transform_dirty();
    }

    const Matrix4 &get_world_model_matrix() noexcept {
        if (is_world_transform_dirty) {
            recaculate_world_transform();
        }
        return world_model_matrix;
    }
    const Matrix3 &get_world_normal_matrix() noexcept {
        if (is_world_transform_dirty) {
            recaculate_world_transform();
        }
        return world_normal_matrix;
    }

    const std::vector<std::shared_ptr<GObject>> &get_children() const noexcept { return children; }
    std::shared_ptr<GObject> get_child_by_name(const std::string &name) noexcept {
        if (name.empty())
            return nullptr;
        for (auto &child : children) {
            if (child->name == name) {
                return child;
            }
        }
        return nullptr;
    }

    void attach_child(const std::shared_ptr<GObject> &child) noexcept {
        assert(!child->has_parent());
        children.push_back(child);
        child->mark_world_transform_dirty();
        child->parent = weak_from_this();
        child->set_world(this->get_world());
    }

    void remove_child(GObject *child) noexcept {
        assert(child);
        auto it = std::ranges::find_if(children, [child](const auto &c) { return c.get() == child; });
        if (it != children.end()) {
            child->parent.reset();
            child->set_world(this->get_world());
            children.erase(it);
        }
    }

    bool has_parent() const noexcept { return !parent.expired(); }

    std::weak_ptr<GObject> get_parent() noexcept { return parent; }

    void attach_parent(const std::shared_ptr<GObject> &new_parent) noexcept {
        if (new_parent == parent.lock())
            return;
        if (new_parent) {
            new_parent->attach_child(shared_from_this());
        } else {
            // 新父节点为空指针
            if (has_parent()) {
                parent.lock()->remove_child(this);
            }
        }
    }

    bool is_disabled() const noexcept { return disabled; }
    bool is_registered() const noexcept { return _is_registered; }

private:
    friend class World;
    /**
     * @brief 设置此Object所在的世界，如果为nullptr，则此Object将不属于任何世界
     * @note 子节点所在的世界应该总是与父节点保持一致，对外部来说只需要设置root节点的world就可以了
     */
    void set_world(World *world) noexcept;
    void do_deferred_update();

private:
    void do_register();
    void do_unregister();

    /**
     * @brief 标记此节点world_transform为脏，同时递归标记子节点，并且触发应有的更新
     */
    void mark_world_transform_dirty() noexcept{
        is_world_transform_dirty = true;

        if (contain(cpnt_update_flag, ComponentUpdateFlag::TRANSFORM)){
            return; // 这基于如下假设：如果当前已经标记ComponentUpdateFlag::TRANSFORM，则所有子节点也一定已经标记过了
        }
        queue_deferred_update(ComponentUpdateFlag::TRANSFORM);
        // 递归标记子节点
        for(auto& child: children){
            child->mark_world_transform_dirty();
        }
    }
    void recaculate_world_transform() noexcept;
    void queue_deferred_update(ComponentUpdateFlag flag) noexcept;
};
} // namespace Goonya