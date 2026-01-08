#pragma once

#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLMesh.h"
#include <vector>

namespace Goonya {

class RenderInstance {
    Ref<GLMesh> mesh;
    std::vector<Ref<Material>> materials;
    std::vector<std::byte> per_object_data;
};

}; // namespace Goonya
