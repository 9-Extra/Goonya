#pragma once

#include "core/world/GObject.h"
#include "function/renderer/Renderer.h"


namespace Goonya {
namespace Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntSkybox : public Component {
public:
    CpntSkybox(uint32_t skybox_mat_id, bool ingore_range = true, const BoundingBox &bbox = BoundingBox())
        : skybox_mat_id(skybox_mat_id),ignore_range(ingore_range), bbox(bbox) {}
    virtual void tick() override {
        assert(get_owner() != nullptr);
        Vector3f pos = position_from_matrix(get_owner()->get_root_transform_matrix());
        if (ignore_range){
            renderer.current_skyboxs.emplace_back(skybox_mat_id, ignore_range);
        } else {
            renderer.current_skyboxs.emplace_back(skybox_mat_id, ignore_range, bbox.offset(pos));
        }
    }

private:
    uint32_t skybox_mat_id;
    bool ignore_range;
    BoundingBox bbox;
};
} // namespace Graphics
} // namespace Goonya