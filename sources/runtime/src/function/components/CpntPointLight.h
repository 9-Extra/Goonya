#pragma once

#include "core/world/GObject.h"
#include "function/renderer/Renderer.h"

namespace Goonya::Graphics {
// 光源组件
class CpntPointLight : public Component {
public:
    CpntPointLight(Vector3f color, float radius) : color(color), radius(radius) {}

    Vector3f color;
    float radius;

    void on_tick() override {
        assert(get_owner() != nullptr);
        renderer.pointlights.emplace_back() = {get_owner()->get_transform().position, color, radius};
    }
};
} // namespace Goonya::Graphics
