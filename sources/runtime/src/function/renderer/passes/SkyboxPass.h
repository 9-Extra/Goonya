#pragma once

#include "core/cgmath.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Mesh.h"
#include "resource/ResMng.h"

namespace Goonya::Graphics {

class SkyBoxPass : public Pass {
private:
    const static unsigned int SKYBOX_TEXTURE_BINDING = 5;
    struct SkyBoxData final {
        Matrix4 skybox_view_perspective_matrix;
    };

    Ref<Buffer> skybox_uniform;
    Ref<Mesh> mesh;

public:
    SkyBoxPass()
        : skybox_uniform(graphics_api->create_buffer(sizeof(SkyBoxData), BufferType::DYNAMIC)),
          mesh(resources.meshes.get("skybox_cube")) {}

    void run(CameraRenderProxy *camera) override;
};
} // namespace Goonya::Graphics