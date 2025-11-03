#pragma once

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
        return Transform{matrix.resolve_position(), matrix.resolve_rotation(), matrix.resolve_scale()};
    }
    static constexpr Transform from_matrix(const Matrix3 &matrix) {
        return Transform{{}, matrix.resolve_rotation(), matrix.resolve_scale()};
    }

    constexpr Vector3f apply_point(Vector3f p) const noexcept { return (p * scale).apply(rotation) + position; }

    constexpr Vector3f forward_direction() const noexcept { return FORWARD.apply(rotation); }

    constexpr Vector3f up_direction() const noexcept { return UP.apply(rotation); }

    constexpr Matrix4 model_matrix() const noexcept {
        return Matrix4{Matrix3::scale(scale) * Matrix3::rotate(rotation)} * Matrix4::translate(position);
    }
    constexpr Matrix3 normal_matrix() const noexcept {
        // 旋转矩阵 * 缩放矩阵的伴随矩阵
        return Matrix3::scale(scale.y * scale.z, scale.x * scale.z, scale.y * scale.z) * Matrix3::rotate(rotation);
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
