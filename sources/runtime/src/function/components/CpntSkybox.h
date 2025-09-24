#pragma once

#include "core/cgmath.h"
#include "core/world/GObject.h"
#include "function/renderer/Renderer.h"

namespace Goonya::Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntSkybox : public Component {
public:
    explicit CpntSkybox(const Ref<Material> &skybox_material, bool ignore_range = true,
                        const BoundingBox &bbox = BoundingBox())
        : skybox_material(skybox_material), ignore_range(ignore_range), bbox(bbox) {}
    void on_tick() override {
        assert(get_owner() != nullptr);
        Vector3f pos = get_owner()->get_world_model_matrix().resolve_translate();
        if (ignore_range) {
            renderer.current_skyboxs.emplace_back(skybox_material, ignore_range);
        } else {
            renderer.current_skyboxs.emplace_back(skybox_material, ignore_range, bbox.offset(pos));
        }
    }

private:
    Ref<Material> skybox_material;
    bool ignore_range;
    BoundingBox bbox;
};
} // namespace Goonya::Graphics