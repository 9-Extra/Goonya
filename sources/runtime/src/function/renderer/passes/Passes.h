#pragma once

#include "function/renderer/RenderProxy/Camera.h"

namespace Goonya::Graphics {
// Pass 基类
class Pass {
public:
    virtual void run(CameraRenderProxy *camera) = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

} // namespace Goonya::Graphics
