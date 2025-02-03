#pragma once

#include "function/renderer/Renderer.h"
#include "core/world/GObject.h"


namespace Goonya {
namespace Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender: public Component{
public:
    virtual void on_tick() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();
        // 更新每一个part
        if (owner.get_dirty_flag()[GObject::DirtyFlag::TRANSFORM_DIRTY]){
            for (RenderItem &p : parts) {
                p.world_model_matrix = owner.get_world_model_matrix() * p.model_matrix;
                p.world_normal_matrix = owner.get_world_normal_matrix() * p.normal_matrix;
            }
        }
        // 提交每一个part
        for (RenderItem &p : parts) {
            renderer.accept(&p);
        }
    }

    void add_part(const RenderItem &part) {
        RenderItem &p = parts.emplace_back(part);
        if (GObject* owner = get_owner();owner != nullptr){    
            p.world_model_matrix = owner->get_world_model_matrix() * p.model_matrix;
            p.world_normal_matrix = owner->get_world_normal_matrix() * p.normal_matrix;
        }
    }
private:
    std::vector<RenderItem> parts;
};
}
}