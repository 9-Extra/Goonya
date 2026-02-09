#pragma once

#include "core/sparse_set.h"
#include "function/renderer/RScene.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

namespace Goonya {
// 光源组件
class CpntPointLight : public Component {
private:
    RScene *scene = nullptr;
    Handle<RLight> h_light;
    Vector3f color;
    float intensity;

public:
    CpntPointLight(Vector3f color, float intensity) : color(color), intensity(intensity) {}

    void on_register() override {
        GN_ASSERT(get_owner() != nullptr);
        scene = get_owner()->get_world()->get_scene();
        h_light = scene->add_light();
        scene->set_light_linear_color(h_light, color, intensity);
        scene->set_light_position(h_light, get_owner()->get_world_model_matrix().resolve_position());
    }

    void on_unregister() override {
        scene->drop_light(h_light);
        scene = nullptr;
    }

    void on_update(ComponentUpdateFlag flag) override {
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            Vector3f position = get_owner()->get_world_model_matrix().resolve_position();
            scene->set_light_position(h_light, position);
        }
    }
};
} // namespace Goonya
