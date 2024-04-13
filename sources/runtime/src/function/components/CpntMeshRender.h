#pragma once

#include "function/graphics/renderer/Renderer.h"
#include "core/world/GObject.h"


namespace Goonya {
namespace Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntMeshRender: public Component{
public:
    virtual void tick() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();
        // 更新每一个part
        if (owner.render_need_update) {
            for (RenderItem &p : parts) {
                p.base_transform = owner.relate_model_matrix * p.model_matrix;
                p.base_normal_matrix = owner.relate_normal_matrix * p.normal_matrix;
            }
            owner.render_need_update = false;
        }
        // 提交每一个part
        for (RenderItem &p : parts) {
            renderer.accept(&p);
        }
    }

    void add_part(const RenderItem &part) {
        RenderItem &p = parts.emplace_back(part);
        if (GObject* owner = get_owner();owner != nullptr){    
            p.base_transform = owner->relate_model_matrix * p.model_matrix;
            p.base_normal_matrix = owner->transform.normal_matrix() * p.normal_matrix;
        }
    }
private:
    std::vector<RenderItem> parts;
};
}
}