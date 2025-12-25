#pragma once

#include "core/cgmath/vector.h"
#include "function/renderer/RenderProxy/Camera.h"

namespace Goonya {
// Pass 基类
struct PassRenderInfo // NOLINT
{
    CameraRenderProxy *camera;
    Viewport viewport;
    Vector2f screen_size;
    float time;
};

class Pass {
public:
    virtual void run(const PassRenderInfo &info) = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

} // namespace Goonya
