#pragma once

#include "runtime/GAssert.h"
#include "vector.h"

#include <cmath>

namespace Goonya {

struct Quaternion {
    float x, y, z, w;
    constexpr Quaternion() : x(0), y(0), z(0), w(1) {}
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    static constexpr Quaternion identity() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1，顺时针旋转（沿着轴看过去）
    static Quaternion from_rotation(Vector3f axis, float angle) {
        GN_ASSERT(is_nearly_equal(axis.length(), 1.0f));
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针外旋（每个分量的旋转都基于初始坐标系），或者等同于ZYX内旋（每旋转一个分量后，下一次旋转基于旋转后的坐标系）
    static Quaternion from_eular(Vector3f eular) {
        return Quaternion::from_rotation({1, 0, 0}, eular.x) * Quaternion::from_rotation({0, 1, 0}, eular.y) *
               Quaternion::from_rotation({0, 0, 1}, eular.z);
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

    constexpr Quaternion operator+(this const Quaternion lhs, Quaternion rhs) noexcept {
        return Quaternion{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
    }

    constexpr Quaternion operator*(this const Quaternion lhs, float rhs) noexcept {
        return Quaternion{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
    }

    constexpr Quaternion operator*(this const Quaternion lhs, Quaternion rhs) noexcept {
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

    constexpr float dot(Quaternion r) const noexcept { return x * r.x + y * r.y + z * r.z + w * r.w; }

    bool operator==(this const Quaternion &self, const Quaternion r) noexcept {
        return is_nearly_equal(self.x, r.x) && is_nearly_equal(self.y, r.y) && is_nearly_equal(self.z, r.z) &&
               is_nearly_equal(self.w, r.w);
    }

    static bool rotation_equal(Quaternion a, Quaternion b) noexcept {
        const float EPSILON = 1e-6f;
        // 检查 q1 是否等于 q2 或 -q2（因为两者代表相同旋转）
        double dot1 = std::abs(a.dot(b));
        double dot2 = std::abs(a.dot(b * -1.0f));

        return (std::abs(1.0f - dot1) < EPSILON) || (std::abs(1.0f - dot2) < EPSILON);
    }

    bool isnan() const noexcept { return std::isnan(x) || std::isnan(y) || std::isnan(z) || std::isnan(w); }

    /**
     * @brief 线性插值（Lerp）
     * 用于在两个四元数之间进行线性插值，快，在四元数相近时效果好。
     *
     * @note 没有归一化，可能需要按情况手动归一化
     * @param q1 起始四元数
     * @param q2 结束四元数
     * @param t 插值参数，范围[0,1]为插值，超过1时预测未来
     * @return Quaternion 插值后的四元数
     */
    static Quaternion lerp(Quaternion q1, Quaternion q2, float t) {
        // 计算两个四元数之间的夹角余弦
        double cosine = q1.dot(q2);

        // 处理负点积的情况，确保插值沿最短路径
        if (cosine < 0.0f) {
            q2 = q2 * -1.0f;
            cosine = -cosine;
        }

        return q1 * (1.0f - t) + q2 * t;
    }

    /**
     * @brief 球面线性插值（Slerp）
     * 用于在两个四元数之间进行球面插值，慢但准确，适用于四元数相差较大时。
     *
     * @param q1 起始四元数
     * @param q2 结束四元数
     * @param t 插值参数，范围[0,1]为插值，超过1时预测未来
     * @return Quaternion 插值后的四元数
     */
    static Quaternion slerp(Quaternion q1, Quaternion q2, float t) {
        // 计算两个四元数之间的夹角余弦
        double cosine = q1.dot(q2);

        // 处理负点积的情况，确保插值沿最短路径
        if (cosine < 0.0f) {
            q2 = q2 * -1.0f;
            cosine = -cosine;
        }

        // 检查是否非常接近
        const float EPSILON = 1e-6;

        if (cosine > 1.0f - EPSILON) {
            // 非常接近，使用线性插值避免除零
            return (q1 * (1.0f - t) + q2 * t).normalize();
        }

        // 标准SLERP公式
        float omega = std::acosf(cosine);
        float sin_omega = std::sinf(omega);

        // 计算插值系数
        float scale0 = std::sinf((1.0f - t) * omega) / sin_omega;
        float scale1 = std::sinf(t * omega) / sin_omega;

        return (q1 * scale0 + q2 * scale1).normalize();
    }

    static float angle_between(Quaternion q1, Quaternion q2) noexcept {
        float dot = std::abs(q1.dot(q2));
        return 2.0f * std::acos(dot);
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
