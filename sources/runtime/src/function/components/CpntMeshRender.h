#pragma once

#include "core/intrusive_ptr.h"
#include "core/world/GObject.h"
#include "function/renderer/Renderer.h"
#include "platform/graphics/Material.h"

namespace Goonya::Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender : public Component {
public:
    intrusive_ptr<Mesh> mesh;
    std::vector<intrusive_ptr<Material>> materials;

    void on_register() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();

        render_info = {
            .model_matrix = owner.get_world_model_matrix(),
            .normal_matrix = owner.get_world_normal_matrix(),
            .mesh = mesh,
            .materials = materials,
        };
        renderer.add_mesh_info(&render_info);
    }

    void on_unregister() override {
        assert(get_owner() != nullptr);
        renderer.remove_mesh_info(&render_info);
    }

    void on_tick() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();

        if (owner.get_dirty_flag()[GObject::DirtyFlag::TRANSFORM_DIRTY]) {
            render_info.model_matrix = owner.get_world_model_matrix();
            render_info.normal_matrix = owner.get_world_normal_matrix();
        }
    }

private:
    MeshRenderInfo render_info;
};
} // namespace Goonya::Graphics
