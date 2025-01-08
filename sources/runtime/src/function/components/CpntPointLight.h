#pragma once

#include "function/renderer/Renderer.h"
#include "core/world/GObject.h"


namespace Goonya {
namespace Graphics {
// 光源组件
class CpntPointLight: public Component{
public:
    CpntPointLight(Vector3f color, float radius): color(color), radius(radius) {}

    Vector3f color;
    float radius;

    virtual void tick() override {
        assert(get_owner() != nullptr);
        renderer.pointlights.emplace_back() = {get_owner()->get_transform().position, color, radius};   
    }
};
}
}
