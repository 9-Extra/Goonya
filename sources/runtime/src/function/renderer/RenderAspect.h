#pragma once

#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/cgmath.h"
#include "platform/graphics/Material.h"

namespace Goonya {

struct PointLight {
    Vector3f position;
    Vector3f color;
    float factor = 1.0f;
};

struct DirectionalLight {
    Vector3f direction;
    Vector3f flux;
};

struct Skybox {
    Ref<GLTexture> env_map;
    Ref<Material> skybox_material;
    bool ignore_range;
    BoundingBox bbox;
};
} // namespace Goonya
