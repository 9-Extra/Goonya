#pragma once

#include "core/plf_colony.h"
#include "function/renderer/RenderAspect.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

namespace Goonya::Graphics {
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
        pointlight_handle = world.main_scene()->pointlights.emplace(get_owner()->get_transform().position, color, radius);
    }

    void on_unregister() override {
        world.main_scene()->pointlights.erase(pointlight_handle);
    }

    void on_tick() override {}

};
} // namespace Goonya::Graphics
