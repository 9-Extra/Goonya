#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "core/cgmath.h"
#include "Component.h"


namespace Goonya {

// 一个物体，单独的物体没有功能，也不可见，需要挂上组件来实现具体的功能
class GObject final : public std::enable_shared_from_this<GObject> {
public:
    GObject(const std::string name = "") noexcept: name(name){};
    GObject(const Transform transform, const std::string name = "") noexcept
        : name(name), transform(transform) {};

    
    ~GObject() {
        assert(!parent.lock()); // 必须没有父节点
    };

    struct DirtyFlag{
        using FlagType = uint32_t;

        FlagType value = DEFAULT;
        DirtyFlag(FlagType value = DEFAULT) noexcept: value(value) {}
        
        const static FlagType DEFAULT = 0;
        const static FlagType TRANSFORM_DIRTY = 1 << 0;

        bool operator[](FlagType ft) const noexcept{
            return (value & ft) == ft;
        }

        bool operator[](DirtyFlag ft) const noexcept{
            return (value & ft.value) == ft.value;
        }

        void append(DirtyFlag f) noexcept{
            this->value |= f.value;
        }
        void append(FlagType f) noexcept{
            this->value |= f;
        }
        void remove(DirtyFlag f) noexcept{
            this->value &= f.value;
        }
        
        void remove(FlagType f) noexcept{
            this->value &= f;
        }

        void clear() noexcept{
            value = DEFAULT;
        }
    };

    void enable() noexcept{
        if (!is_disabled()) return;
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY); // 重新计算变换矩阵
        disabled = false;
        for (auto &component : components) {
            component->on_register();
        }
        for (std::shared_ptr<GObject>& child: children){
            child->enable();
        }
    }

    void disable() noexcept{
        if (is_disabled()) return;
        disabled = true;
        for (auto &component : components) {
            component->on_unregister();
        }
        for (std::shared_ptr<GObject>& child: children){
            child->disable();
        }
    }

    void add_component(std::unique_ptr<Component>&& component) {
        component->set_owner(this);
        component->on_register();
        components.push_back(std::move(component));
    }

    Component* get_component(const std::type_info& t_info) noexcept{
        for (auto &component : components) {
            auto& c = *component;// 比较其内容而非智能指针
            if (typeid(c) == t_info) {
                return component.get();
            }
        }
        return nullptr;
    }

    template<class T> requires(std::is_base_of_v<Component, T>)
    T* get_component() noexcept{
        return (T*)get_component(typeid(T));
    }

    bool remove_component(const std::type_info& t_info) {
        for (auto it = components.begin(); it!= components.end(); ++it) {
            auto& c = **it;// 比较其内容而非智能指针
            c.on_unregister();
            c.set_owner(nullptr);
            if (typeid(c) == t_info) {
                components.erase(it);
                return true;
            }
        }
        return false;
    }

    template<class T> requires(std::is_base_of_v<Component, T>)
    bool remove_component() {
        return remove_component(typeid(T));
    }

    std::vector<std::unique_ptr<Component>>& get_components() noexcept{
        return components;
    }

    void set_transform(const Transform &transform) noexcept{
        this->transform = transform;
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
    }

    const Transform& get_transform() const noexcept{
        return transform;
    }

    void translate(Vector3f distance) noexcept{
        this->transform.position += distance;
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
    }

    void set_position(Vector3f pos) noexcept{
        this->transform.position = pos;
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
    }

    void rotate_local_axis(Vector3f angle) noexcept{
        rotate_local_axis(Quaternion::from_eular(angle));
    }

    void rotate_local_axis(Quaternion rotation) noexcept{
        this->transform.rotation *= rotation;
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
    }

    
    void rotate_global_axis(Vector3f angle) noexcept{
        rotate_global_axis(Quaternion::from_eular(angle));
    }

    void rotate_global_axis(Quaternion rotation) noexcept{
        // 沿全局坐标系旋转，依赖于父节点相对世界的旋转
        // todo: 如果父节点的world_model_matrix是脏的怎么办？
        if (has_parent()){
            Quaternion parent_rotation = Quaternion::from_matrix(parent.lock()->world_model_matrix);
            this->transform.rotation = parent_rotation * rotation * parent_rotation.conjugate() * this->transform.rotation;
        } else {
            this->transform.rotation = this->transform.rotation * rotation;
        }
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
    }

    void set_rotation(Quaternion rotation) noexcept{
        this->transform.rotation = rotation;
        dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
    }
    DirtyFlag get_dirty_flag() const noexcept{
        return dirty_flag;
    }

    const Matrix4& get_world_model_matrix() const noexcept{
        return world_model_matrix;
    }
    const Matrix3& get_world_normal_matrix() const noexcept{
        return world_normal_matrix;
    }

    const std::vector<std::shared_ptr<GObject>> &get_children() const noexcept{ return children; }
    std::shared_ptr<GObject> get_child_by_name(const std::string name) noexcept{
        if (name.empty())
            return nullptr;
        for (auto &child : children) {
            if (child->name == name) {
                return child;
            }
        }
        return nullptr;
    }

    void attach_child(std::shared_ptr<GObject> child) noexcept{
        if (auto old_parent = child->parent.lock(); old_parent) {
            old_parent->remove_child(child.get());
        }
        children.push_back(child);
        child->dirty_flag.append(DirtyFlag::TRANSFORM_DIRTY);
        child->parent = weak_from_this();
    }

    void remove_child(GObject *child) noexcept{
        auto it =
            std::find_if(children.begin(), children.end(), [child](const auto &c) -> bool { return c.get() == child; });
        if (it != children.end()) {
            (*it)->parent.reset();
            children.erase(it);
        } else {
            assert(false); // 试图移除不存在的子节点
        }
    }

    bool has_parent() const noexcept{ return !parent.expired(); }

    std::weak_ptr<GObject> get_parent() noexcept{ return parent; }

    void attach_parent(std::shared_ptr<GObject> new_parent) noexcept{
        if (new_parent == parent.lock())
            return;
        if (new_parent){
            new_parent->attach_child(shared_from_this());
        } else {
            parent.lock()->remove_child(this);
        }
    }

    bool is_disabled() noexcept{
        return disabled;
    }

private:
    friend class World;
    std::string name;
    bool disabled = false;

    Transform transform;  // 相对父节点的变换

    Matrix4 world_model_matrix;  // 世界根节点的变换
    Matrix3 world_normal_matrix; // 世界根节点的法线变换

    std::vector<std::unique_ptr<Component>> components;
    std::weak_ptr<GObject> parent;
    std::vector<std::shared_ptr<GObject>> children;

    DirtyFlag dirty_flag{DirtyFlag::TRANSFORM_DIRTY};

    void tick(DirtyFlag parent_flag);
};
}