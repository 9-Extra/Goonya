#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "core/world/GObject.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/Renderer.h"
#include "function/renderer/RendererBasic.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include <cstdint>

namespace Goonya::Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender : public Component {
public:
    void on_register() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();

        // 初始化时更新所有数据
        is_mesh_dirty = false;
        is_materials_dirty = false;
        enqueue_render_task([=, proxy = &mesh_proxy, copy_mesh = mesh, copy_materials = materials,
                                      model_matrix = owner.get_world_model_matrix(),
                                      normal_matrix = owner.get_world_normal_matrix()] mutable {
            ASSERT_RENDER_THREAD();
            {
                proxy->mesh = copy_mesh;
                proxy->materials = copy_materials;
                proxy->model_matrix = model_matrix.transpose();
                proxy->normal_matrix = Matrix4{normal_matrix.transpose()};
            }
            renderer.add_mesh_proxy(proxy);
        });
    }

    const intrusive_ptr<Mesh> &get_mesh() const noexcept { return mesh; }
    const std::vector<intrusive_ptr<Material>> &get_materials() const noexcept { return materials; }

    void set_mesh(const intrusive_ptr<Mesh> &mesh) noexcept {
        this->mesh = mesh;
        is_mesh_dirty = true;
    }

    void set_material(const intrusive_ptr<Material> &material, uint32_t slot = 0) noexcept {
        if (materials.size() <= slot) {
            materials.resize(slot + 1, nullptr);
        }
        materials[slot] = material;
        is_materials_dirty = true;
    }

    void set_materials(std::span<intrusive_ptr<Material>> materials) noexcept {
        this->materials.assign(materials.begin(), materials.end());
        is_materials_dirty = true;
    }

    void on_unregister() override {
        assert(get_owner() != nullptr);
        enqueue_render_task([proxy = &mesh_proxy]{
            renderer.remove_mesh_proxy(proxy);
        });
    }

    void on_tick() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();

        bool is_transform_dirty = owner.has_dirty_flag(GObject::DirtyFlag::TRANSFORM_DIRTY);

        MeshRenderProxy *proxy = &mesh_proxy;
        if (is_materials_dirty) {
            enqueue_render_task([=, copy_materials = materials] mutable {
                ASSERT_RENDER_THREAD();
                proxy->materials = std::move(copy_materials);
            });
            is_materials_dirty = false;
        }
        if (is_mesh_dirty) {
            enqueue_render_task([=, copy_mesh = mesh] mutable {
                ASSERT_RENDER_THREAD();
                proxy->mesh = copy_mesh;
            });
            is_mesh_dirty = false;
        }
        if (is_transform_dirty) {
            enqueue_render_task([=, model_matrix = owner.get_world_model_matrix(),
                                          normal_matrix = owner.get_world_normal_matrix()] mutable {
                ASSERT_RENDER_THREAD();
                proxy->model_matrix = model_matrix.transpose();
                proxy->normal_matrix = Matrix4{normal_matrix.transpose()};
            });
            is_transform_dirty = false;
        }
    }

protected:
    intrusive_ptr<Mesh> mesh;
    std::vector<intrusive_ptr<Material>> materials;
    bool is_mesh_dirty = false;
    bool is_materials_dirty = false;

    MeshRenderProxy mesh_proxy;
};
} // namespace Goonya::Graphics
