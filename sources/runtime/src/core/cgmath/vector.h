#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <type_traits>

namespace Goonya {

template <typename T>
    requires std::is_arithmetic_v<T>
constexpr int8_t sign(T x) {
    if constexpr (std::is_signed_v<T>) {
        return x > 0 ? 1 : (x < 0 ? -1 : 0);
    } else if constexpr (std::is_unsigned_v<T>) {
        return x > 0 ? 1 : 0;
    } else {
        static_assert(std::is_void_v<T>, "what?!");
    }
}

template <std::floating_point T>
constexpr bool is_nearly_equal(T a, T b, T bias = 0.00001) noexcept {
    if (a == b) {
        return true;
    } else {
        return std::abs(a - b) < bias;
    }
}

struct Vector2f {
    union {
        struct {
            float x, y;
        };
        float v[2]{};
    };
    constexpr Vector2f() : x(0), y(0) {}
    constexpr Vector2f(float x, float y) : x(x), y(y) {}

    constexpr Vector2f operator+(const Vector2f b) const { return {x + b.x, y + b.y}; }

    constexpr Vector2f operator-(const Vector2f b) const { return {x - b.x, y - b.y}; }

    constexpr Vector2f operator*(const float s) const { return {x * s, y * s}; }

    constexpr float squared() const { return x * x + y * y; }

    float length() const { return sqrtf(squared()); }

    Vector2f normalized() const {
        float s = 1.0f / std::sqrt(squared());
        return {x * s, y * s};
    }

    constexpr Vector2f rotate(float radian) const {
        return {x * cosf(radian) + y * sinf(radian), x * -sinf(radian) + y * cosf(radian)};
    }
};

struct Color {
    float r, g, b, a = 1.0f;
    constexpr Color() : r(0), g(0), b(0), a(0) {}
    constexpr Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    constexpr bool operator==(const Color &ps) const { return r == ps.r && g == ps.g && b == ps.b && a == ps.a; }

    constexpr bool operator!=(const Color &ps) const { return !(*this == ps); }
};

struct Matrix3;
struct Matrix4;
struct Quaternion;

struct Vector3f {
    union {
        struct {
            float x, y, z;
        };
        float v[3]{};
    };

    constexpr Vector3f() : x(0), y(0), z(0) {}
    constexpr Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}

    const float *data() const { return v; }

    constexpr float &operator[](unsigned int i) noexcept { return v[i]; }
    constexpr const float &operator[](unsigned int i) const noexcept { return v[i]; }

    constexpr Vector3f operator+(Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }
    constexpr Vector3f operator-(Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }
    constexpr Vector3f operator-() const { return Vector3f{-x, -y, -z}; }
    constexpr Vector3f operator+=(Vector3f b) { return *this = *this + b; }
    constexpr Vector3f operator*(float n) const { return {x * n, y * n, z * n}; }
    constexpr Vector3f operator/(float n) const { return *this * (1.0f / n); }
    constexpr Vector3f operator*(Vector3f v) const { return {x * v.x, y * v.y, z * v.z}; }
    constexpr bool operator==(Vector3f rhs) const noexcept {
        return is_nearly_equal(x, rhs.x) && is_nearly_equal(y, rhs.y) && is_nearly_equal(y, rhs.y);
    }
    friend constexpr Vector3f operator*(Vector3f left, Matrix3 right) noexcept;
    constexpr float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    constexpr Vector3f cross(const Vector3f b) const {
        return {this->y * b.z - this->z * b.y, this->z * b.x - this->x * b.z, this->x * b.y - this->y * b.x};
    }
    inline constexpr Vector3f apply(Quaternion q) const noexcept;

    constexpr float square() const { return this->dot(*this); }
    float length() const { return std::sqrt(square()); }
    Vector3f normalize() const {
        float inv_sqrt = 1.0f / length();
        return *this * inv_sqrt;
    }
};

constexpr static Vector3f FORWARD = {0, 0, -1};
constexpr static Vector3f UP = {0, 1, 0};
constexpr static Vector3f RIGHT = {1, 0, 0};

struct Vector4f {
    union {
        struct {
            float x, y, z, w;
        };
        float v[4]{};
    };

    constexpr Vector4f() : x(0), y(0), z(0), w(0) {}
    constexpr Vector4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    constexpr Vector4f(Vector3f vec3, float w) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {}

    constexpr Vector3f get_xyz() const noexcept { return Vector3f{x, y, z}; }

    constexpr float &operator[](size_t i) noexcept { return v[i]; }
    constexpr const float &operator[](size_t i) const noexcept { return v[i]; }
    constexpr Vector4f operator+(Vector4f n) const noexcept { return {x + n.x, y + n.y, z + n.z, w + n.w}; }
    constexpr Vector4f operator-(Vector4f n) const noexcept { return {x - n.x, y - n.y, z - n.z, w - n.w}; }
    constexpr Vector4f operator*(const float n) const noexcept { return {x * n, y * n, z * n, w * n}; }
    constexpr Vector4f operator/(const float n) const noexcept { return *this * (1.0f / n); }

    friend constexpr Vector4f operator*(Vector4f left, Matrix4 right) noexcept;
};

} // namespace Goonya

template <>
struct std::formatter<Goonya::Vector2f> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Vector2f vec2, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {})", vec2.x, vec2.y);
    }
};

template <>
struct std::formatter<Goonya::Vector3f> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Vector3f vec3, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", vec3.x, vec3.y, vec3.z);
    }
};
