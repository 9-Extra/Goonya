#pragma once

#include "core/RefCount.h"
#include "core/cgmath/cgmath.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RendererBasic.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "function/world/World.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <ranges>

namespace Goonya::Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender : public Component {
protected:
    Ref<Mesh> mesh;
    std::vector<Ref<Material>> materials;

    MeshRenderProxy *mesh_proxy;

public:
    void on_register() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();

        // 初始化时更新所有数据
        mesh_proxy = new MeshRenderProxy();
        mesh_proxy->mesh = mesh;
        mesh_proxy->materials = materials;
        mesh_proxy->model_matrix = owner.get_world_model_matrix();
        mesh_proxy->normal_matrix = owner.get_world_normal_matrix();
        mesh_proxy->aabbs.reserve(mesh->submeshes.size());
        for (SubMesh &sub_mesh : mesh->submeshes) {
            mesh_proxy->aabbs.push_back(sub_mesh.aabb.transformed(owner.get_world_model_matrix()));
        }

        enqueue_render_task([mesh_proxy = mesh_proxy, &scene = get_owner()->get_world()->main_scene()] mutable {
            ASSERT_RENDER_THREAD();
            // mesh_proxy移交给渲染线程，Component中只持有指针，不要在逻辑线程访问它
            scene.mesh_proxys.emplace(std::unique_ptr<MeshRenderProxy>{mesh_proxy});
        });
    }

    const Ref<Mesh> &get_mesh() const noexcept { return mesh; }
    const std::vector<Ref<Material>> &get_materials() const noexcept { return materials; }

    void set_mesh(const Ref<Mesh> &mesh) noexcept {
        this->mesh = mesh;
        if (mesh_proxy) {
            enqueue_render_task([proxy = mesh_proxy, mesh] mutable { proxy->mesh = mesh; });
        }
    }

    void set_material(uint32_t slot, const Ref<Material> &material) noexcept {
        if (materials.size() <= slot) {
            materials.resize(slot + 1, nullptr);
        }
        materials[slot] = material;
        if (mesh_proxy) {
            enqueue_render_task([proxy = mesh_proxy, slot, material] mutable {
                proxy->materials.resize(slot + 1, nullptr);
                proxy->materials[slot] = material;
            });
        }
    }

    void set_materials(std::span<Ref<Material>> materials) noexcept {
        this->materials.assign(materials.begin(), materials.end());
        if (mesh_proxy) {
            enqueue_render_task([proxy = mesh_proxy, mats = this->materials] mutable { proxy->materials = mats; });
        }
    }

    void on_unregister() override {
        assert(get_owner() != nullptr);
        RenderScene *scene = &get_owner()->get_world()->main_scene();
        enqueue_render_task([proxy = mesh_proxy, scene] {
            auto &container = scene->mesh_proxys;
            auto iter = container.find(proxy);
            assert(iter != container.end());
            container.erase(iter);
        });
    }

    void on_update(ComponentUpdateFlag flag) override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();
        
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            enqueue_render_task([proxy = mesh_proxy, model_matrix = owner.get_world_model_matrix(),
                                 normal_matrix = owner.get_world_normal_matrix()] mutable {
                ASSERT_RENDER_THREAD();
                proxy->model_matrix = model_matrix;
                proxy->normal_matrix = normal_matrix;
                for (const auto &[i, sub_mesh] : std::views::enumerate(proxy->mesh->submeshes)) {
                    proxy->aabbs[i] = sub_mesh.aabb.transformed(model_matrix);
                }
            });
        }
    }
};

} // namespace Goonya::Graphics
