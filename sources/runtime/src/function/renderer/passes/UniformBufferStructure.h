#pragma once

#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"

namespace Goonya {

static constexpr unsigned int POINTLIGHT_MAX = 8;

struct PointLightData final {
    alignas(16) Vector3f position;
    alignas(16) Vector3f intensity;
};

struct PerFrameData final { // NOLINT：不需要初始化
    Matrix4f view_perspective_matrix;
    alignas(16) Vector3f ambient_light;
    alignas(16) Vector3f camera_position;
    alignas(4) float fog_min_distance;
    alignas(4) float fog_density;
    alignas(4) float time;
    alignas(4) Vector2f screen_size;
    alignas(4) uint32_t pointlight_num;
    PointLightData pointlight_list[POINTLIGHT_MAX];
};

struct alignas(256) PerObjectData final {
    Matrix4f model_matrix;
    Matrix4f normal_matrix; // 内存对齐
};

} // namespace Goonya
