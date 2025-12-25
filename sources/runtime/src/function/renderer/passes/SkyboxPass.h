#pragma once

#include "core/cgmath/cgmath.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/ResMng.h"

namespace Goonya {

class SkyBoxPass : public Pass {
private:
    const static unsigned int SKYBOX_TEXTURE_BINDING = 5;
    struct SkyBoxData final {
        Matrix4 skybox_view_perspective_matrix;
    };

    Ref<GLBuffer> skybox_uniform;
    Ref<GLMesh> mesh;

public:
    SkyBoxPass()
        : skybox_uniform(create_ref<GLBuffer>(sizeof(SkyBoxData), BufferType::DYNAMIC)),
          mesh(resources.load_resource<GLMesh>("buildin:skybox_cube")) {}

    void run(const PassRenderInfo &info) override;
};
} // namespace Goonya
