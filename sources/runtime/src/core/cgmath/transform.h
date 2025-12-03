#pragma once

#include "core/cgmath/vector.h"
#include "matrix.h"

namespace Goonya {

struct Transform {
    Vector3f position;
    Quaternion rotation;
    Vector3f scale;

    constexpr explicit Transform(Vector3f position = {0, 0, 0}, Quaternion rotation = Quaternion::identity(),
                                 Vector3f scale = {1, 1, 1})
        : position(position), rotation(rotation), scale(scale) {}
    static constexpr Transform from_matrix(const Matrix4 &matrix) {
        Vector3f pos = matrix.resolve_position();
        Vector3f scale = matrix.resolve_scale();
        Matrix4 normalized = Matrix4::identity().scale({1 / scale.x, 1 / scale.y, 1 / scale.z}) * matrix;
        Quaternion rotation = normalized.resolve_rotation_normalized();
        return Transform{pos, rotation, scale};
    }
    static constexpr Transform from_matrix(const Matrix3 &matrix) {
        Vector3f scale = matrix.resolve_scale();
        Matrix3 normalized = matrix.scale({1 / scale.x, 1 / scale.y, 1 / scale.z});
        Quaternion rotation = normalized.resolve_rotation_normalized();
        return Transform{{0, 0, 0}, rotation, scale};
    }

    constexpr Vector3f apply_point(Vector3f p) const noexcept { return (p * scale).apply(rotation) + position; }

    constexpr Vector3f forward_direction() const noexcept { return FORWARD.apply(rotation); }

    constexpr Vector3f up_direction() const noexcept { return UP.apply(rotation); }

    constexpr Matrix4 model_matrix() const noexcept {
        return Matrix4{Matrix3::identity().scale(scale).rotate(rotation)}.translate(position);
    }
    constexpr Matrix3 normal_matrix() const noexcept {
        // 缩放矩阵的伴随矩阵 * 旋转矩阵
        return Matrix3::identity().scale({scale.y * scale.z, scale.x * scale.z, scale.y * scale.z}).rotate(rotation);
    }
};

struct Plane {
    // 完整的平面方程是: normal.x * x + normal.y * y + normal.z * z + d = 0
    Vector3f normal;
    float d = 0;

    Plane() noexcept = default;
    Plane(Vector3f normal, float d) noexcept : normal(normal), d(d) {}
    explicit Plane(Vector4f vec4) noexcept : normal(vec4.get_xyz()), d(vec4.w) {}

    Plane normalize() const noexcept {
        float len = normal.length();
        return Plane{normal / len, d / len};
    }
};

} // namespace Goonya
