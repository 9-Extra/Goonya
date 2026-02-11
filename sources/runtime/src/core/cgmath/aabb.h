#pragma once

#include "matrix.h"

namespace Goonya {

struct BoundingBox {
    Vector3f min;
    Vector3f max;

    constexpr static BoundingBox infinite() {
        // 不使用浮点inf作为包围盒长度，因为计算center会得到nan
        constexpr float max = std::numeric_limits<float>::max();
        constexpr float min = std::numeric_limits<float>::min();
        return BoundingBox{{-min, -min, -min}, {max, max, max}};
    }

    constexpr BoundingBox() noexcept : min{0, 0, 0}, max{0, 0, 0} {};
    constexpr BoundingBox(Vector3f min, Vector3f max) noexcept : min(min), max(max) {
        GN_ASSERT(min.x <= max.x && min.y <= max.y && min.z <= max.z);
    }

    constexpr BoundingBox offset(Vector3f pos) const noexcept { return BoundingBox{min + pos, max + pos}; }
    /**
     * @brief 获取变换后后的保守包围盒
     */
    constexpr BoundingBox transformed(const Matrix4f &transform) const noexcept {
        /*
        此函数源自 [Godot] (https://github.com/godotengine/godot)
        版权归 [Juan Linietsky, Ariel Manzur] 所有
        遵循 MIT 许可证
        */
        Matrix3f rotation_scale = transform.to_matrix3().transpose();
        Vector3f position = transform.resolve_position();

        Vector3f tmin, tmax;
        for (int i = 0; i < 3; i++) {
            tmin[i] = position[i];
            tmax[i] = position[i];

            for (int j = 0; j < 3; j++) {
                float e = rotation_scale.m[i][j] * min[j];
                float f = rotation_scale.m[i][j] * max[j];
                if (e < f) {
                    tmin[i] += e;
                    tmax[i] += f;
                } else {
                    tmin[i] += f;
                    tmax[i] += e;
                }
            }
        }

        return {tmin, tmax};
    }
    constexpr bool contains(Vector3f pos) const noexcept {
        return pos.x >= min.x && pos.y >= min.y && pos.z >= min.z && pos.x <= max.x && pos.y <= max.y && pos.z <= max.z;
    }
    constexpr float volume() const noexcept {
        Vector3f vec = max - min;
        return vec.x * vec.y * vec.z;
    }
    constexpr Vector3f center() const noexcept { return (min + max) * 0.5f; }
    constexpr Vector3f extent() const noexcept { return (max - min) * 0.5f; }
};

} // namespace Goonya