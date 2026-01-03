#pragma once

#include "core/cgmath/vector.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLTexture.h"

namespace Goonya {
// Pass 基类
struct PassRenderInfo // NOLINT
{
    CameraRenderProxy *camera;
    Viewport viewport;
    Vector2f screen_size;
    Ref<GLTexture> env_map;
    Ref<Material> skybox_material;
    float time;
};

class Pass {
public:
    virtual void run(PassRenderInfo &info) = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

} // namespace Goonya
