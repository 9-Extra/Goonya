#pragma once

#include "core/RefCount.h"
#include "function/renderer/Material.h"
#include "function/renderer/RScene.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Goonya {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender : public Component {
private:
    RScene *scene = nullptr;
    RenderableRef renderable;

    Ref<Mesh> mesh;
    std::vector<Ref<Material>> materials;

public:
    std::unique_ptr<Component> clone() const override {
        auto new_comp = std::make_unique<CpntMeshRender>();
        new_comp->mesh = mesh;
        new_comp->materials = materials;
        return new_comp;
    }

    void on_register() override {
        GN_ASSERT(get_owner() != nullptr);
        GObject &owner = *get_owner();
        scene = owner.get_world()->get_scene();
        renderable = RenderableRef{*scene};
        renderable.set_transform(owner.get_world_model_matrix());
        if (mesh) {
            renderable.set_mesh(mesh);
        }
        if (!materials.empty()) {
            renderable.set_materials(materials);
        }
    }

    const Ref<Mesh> &get_mesh() const noexcept { return mesh; }
    const std::vector<Ref<Material>> &get_materials() const noexcept { return materials; }

    void set_mesh(const Ref<Mesh> &mesh) noexcept {
        this->mesh = mesh;
        if (renderable.is_valid()) {
            renderable.set_mesh(mesh);
        }
    }

    void set_material(uint32_t slot, const Ref<Material> &material) noexcept {
        if (materials.size() <= slot) {
            materials.resize(slot + 1, nullptr);
        }
        materials[slot] = material;
        if (renderable.is_valid()) {
            renderable.set_materials(materials);
        }
    }

    void set_materials(std::span<Ref<Material>> materials) noexcept {
        this->materials.assign_range(materials);
        if (renderable.is_valid()) {
            renderable.set_materials(this->materials);
        }
    }

    void on_unregister() override {
        renderable = {};
        scene = nullptr;
    }

    void on_update(ComponentUpdateFlag flag) override {
        GN_ASSERT(renderable.is_valid());

        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            renderable.set_transform(get_owner()->get_world_model_matrix());
        }
    }
};

} // namespace Goonya
