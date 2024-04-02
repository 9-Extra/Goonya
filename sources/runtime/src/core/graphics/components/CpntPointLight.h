#pragma once

#include "../renderer/Renderer.h"
#include "core/world/GObject.h"

// 光源组件
class CpntPointLight: public Component{
public:
    CpntPointLight(Vector3f color, float radius): color(color), radius(radius) {}

    Vector3f color;
    float radius;

    virtual void tick() override {
        assert(get_owner() != nullptr);
        renderer.pointlights.emplace_back() = {get_owner()->transform.position, color, radius};   
    }
};

