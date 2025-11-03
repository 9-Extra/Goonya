#pragma once

#include "core/cgmath/aabb.h"
#include "core/cgmath/cgmath.h"
#include "core/RefCount.h"
#include "platform/graphics/Material.h"

namespace Goonya::Graphics {

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
    Ref<Material> material;
    bool ignore_range;
    BoundingBox bbox;
};

/**
 * @brief 着色器中物体的Uniform Block的内存布局
 * 
 */
struct PerObjectBuffer{
    Matrix4 model_matrix;
    Matrix4 normal_matrix;
};

} // namespace Goonya::Graphics