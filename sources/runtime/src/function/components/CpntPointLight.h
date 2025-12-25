#pragma once

#include "core/plf_colony.h"
#include "function/renderer/RenderAspect.h"
#include "function/world/GObject.h"
#include "function/world/World.h"
#include "platform/graphics/Graphics.h"

namespace Goonya {
// 光源组件
class CpntPointLight : public Component {
private:
    plf::colony<PointLight>::iterator pointlight_handle;
public:
    CpntPointLight(Vector3f color, float radius) : color(color), radius(radius) {}

    Vector3f color;
    float radius;

    void on_register() override {
        assert(get_owner() != nullptr);
        RenderScene *scene = &get_owner()->get_world()->main_scene();
        pointlight_handle = scene->pointlights.emplace(get_owner()->get_local_transform().position, color, radius);
    }

    void on_unregister() override {
        RenderScene *scene = &get_owner()->get_world()->main_scene();
        scene->pointlights.erase(pointlight_handle);
    }

    void on_update(ComponentUpdateFlag flag) override {
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            Vector3f position = get_owner()->get_world_model_matrix().resolve_position();
            enqueue_render_task([handle = pointlight_handle, position]{
                handle->position = position;
            });
        }
    }

};
} // namespace Goonya
