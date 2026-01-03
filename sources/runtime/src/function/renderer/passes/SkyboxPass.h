#pragma once

#include "function/renderer/passes/Passes.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/ResMng.h"

namespace Goonya {

class SkyBoxPass : public Pass {
private:
    const static unsigned int SKYBOX_TEXTURE_BINDING = 5;

    Ref<GLMesh> mesh;

public:
    SkyBoxPass() : mesh(resources.load_resource<GLMesh>("buildin:skybox_cube")) {}

    void run(PassRenderInfo &info) override;
};
} // namespace Goonya
