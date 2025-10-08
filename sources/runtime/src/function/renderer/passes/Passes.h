#pragma once

#include "function/renderer/RenderProxy/Camera.h"
#include "platform/graphics/RenderTarget.h"

namespace Goonya::Graphics {
// Pass 基类
struct PassRenderInfo{
    CameraRenderProxy *camera;
    Viewport viewport;
    float width_height_ratio;
};

class Pass {
public:
    virtual void run(const PassRenderInfo& info) = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

} // namespace Goonya::Graphics
