#pragma once

#include "core/cgmath.h"
#include "core/world/GObject.h"
#include "function/renderer/Renderer.h"


namespace Goonya {
namespace Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntSkybox : public Component {
public:
    CpntSkybox(intrusive_ptr<Graphics::Material> skybox_material, bool ingore_range = true, const BoundingBox &bbox = BoundingBox())
        : skybox_material(skybox_material),ignore_range(ingore_range), bbox(bbox) {}
    virtual void on_tick() override {
        assert(get_owner() != nullptr);
        Vector3f pos = get_owner()->get_world_model_matrix().resolve_translate();
        if (ignore_range){
            renderer.current_skyboxs.emplace_back(skybox_material, ignore_range);
        } else {
            renderer.current_skyboxs.emplace_back(skybox_material, ignore_range, bbox.offset(pos));
        }
    }

private:
    intrusive_ptr<Graphics::Material> skybox_material;
    bool ignore_range;
    BoundingBox bbox;
};
} // namespace Graphics
} // namespace Goonya