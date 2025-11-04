#pragma once

#include "vector.h"

#include <cassert>
#include <cmath>

namespace Goonya {

struct Quaternion {
    float x, y, z, w;
    constexpr Quaternion() : x(0), y(0), z(0), w(1) {}
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    static constexpr Quaternion identity() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1，顺时针旋转（沿着轴看过去）
    static Quaternion from_rotation(Vector3f axis, float angle) {
        assert(is_nearly_equal(axis.length(), 1.0f));
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针外旋（每个分量的旋转都基于初始坐标系），或者等同于ZYX内旋（每旋转一个分量后，下一次旋转基于旋转后的坐标系）
    static Quaternion from_eular(Vector3f rotate) {
        return Quaternion::from_rotation({1, 0, 0}, rotate.x) * Quaternion::from_rotation({0, 1, 0}, rotate.y) *
               Quaternion::from_rotation({0, 0, 1}, rotate.z);
    }

    /**
     * @brief 级联旋转
     * 使用乘法进行旋转顺序与逻辑顺序相反，而apply是正的。对向量v进行旋转A，B，C使用乘法写作
     * C * B * A * v
     * 但使用apply则写作
     * v.apply(A.apply(B).apply(C))
     * 更加似人
     * @param next
     * @return constexpr Quaternion
     */
    constexpr Quaternion apply(Quaternion next) const noexcept { return next * (*this); }

    constexpr Vector3f forward_direction() const noexcept { return FORWARD.apply(*this); }

    constexpr Vector3f up_direction() const noexcept { return UP.apply(*this); }
    float length() const noexcept { return std::sqrtf(x * x + y * y + z * z + w * w); }

    Quaternion normalize() const noexcept {
        float s = 1.0f / length();
        return Quaternion{x * s, y * s, z * s, w * s};
    }

    constexpr Quaternion conjugate() const { return Quaternion{-x, -y, -z, w}; }

    Quaternion operator*(this const Quaternion lhs, Quaternion rhs) noexcept {
        Vector3f lv{lhs.x, lhs.y, lhs.z}, rv{rhs.x, rhs.y, rhs.z};
        Vector3f v = lv.cross(rv) + lv * rhs.w + rv * lhs.w;
        return Quaternion{v.x, v.y, v.z, lhs.w * rhs.w - lv.dot(rv)};
    }

    Quaternion operator*=(const Quaternion r) noexcept {
        *this = *this * r;
        return *this;
    }

    constexpr Vector3f operator*(Vector3f src) const noexcept {
        // from godot: https://github.com/godotengine/godot
        Vector3f u{x, y, z};
        Vector3f uv = u.cross(src);
        return src + ((uv * w) + u.cross(uv)) * 2.0f;
    }

    bool operator==(this const Quaternion &self, const Quaternion r) noexcept {
        return is_nearly_equal(self.x, r.x) && is_nearly_equal(self.y, r.y) && is_nearly_equal(self.z, r.z) &&
               is_nearly_equal(self.w, r.w);
    }

    bool isnan() const noexcept{
        return std::isnan(x) || std::isnan(y) || std::isnan(z) || std::isnan(w);
    }
};

constexpr Vector3f Vector3f::apply(Quaternion q) const noexcept { return q * (*this); }

} // namespace Goonya

template <>
struct std::formatter<Goonya::Quaternion> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Quaternion q, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", q.x, q.y, q.z, q.w);
    }
};
