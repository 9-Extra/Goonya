#pragma once

#include "core/RefCount.h"
#include "core/cgmath/transform.h"
#include "function/renderer/IMeshRenderable.h"
#include "function/renderer/Material.h"
#include "function/renderer/RScene.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "function/world/World.h"
#include "platform/graphics/opengl/GLMesh.h"

#include <cstdint>
#include <vector>

namespace Goonya {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender : public Component, public IMeshRenderable {
private:
    RScene *scene = nullptr;

public:
    void on_register() override {
        GN_ASSERT(get_owner() != nullptr);
        GObject &owner = *get_owner();
        scene = owner.get_world()->get_scene();
        transform = Transform::from_matrix(owner.get_world_model_matrix());
        scene->register_mesh(this);
    }

    const Ref<GLMesh> &get_mesh() const noexcept { return mesh; }
    const std::vector<Ref<Material>> &get_materials() const noexcept { return materials; }

    void set_mesh(const Ref<GLMesh> &mesh) noexcept {
        this->mesh = mesh;
        this->mark_dirty(DirtyBit::Mesh);
    }

    void set_material(uint32_t slot, const Ref<Material> &material) noexcept {
        if (materials.size() <= slot) {
            materials.resize(slot + 1, nullptr);
        }
        materials[slot] = material;
        this->mark_dirty(DirtyBit::Material);
    }

    void set_materials(std::span<Ref<Material>> materials) noexcept {
        this->materials.assign_range(materials);
        this->mark_dirty(DirtyBit::Material);
    }

    void on_unregister() override {
        GN_ASSERT(get_owner() != nullptr);
        scene->unregister_mesh(this);
        scene = nullptr;
    }

    void on_update(ComponentUpdateFlag flag) override {
        GN_ASSERT(is_registered());
        GObject &owner = *get_owner();

        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            transform = Transform::from_matrix(owner.get_world_model_matrix());
            this->mark_dirty(DirtyBit::Transform);
        }
    }
};

} // namespace Goonya
