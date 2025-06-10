#pragma once

#include <cassert>
#include <cmath>
#include <format>

namespace Goonya {

constexpr float to_radian(float angle) { return angle / 180.0f * 3.1415926535f; }

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

    constexpr float operator[](const unsigned int i) const { return v[i]; }

    constexpr Vector3f operator+(const Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }
    constexpr Vector3f operator-(const Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }
    constexpr Vector3f operator-() const { return Vector3f{-x, -y, -z}; }
    constexpr Vector3f operator+=(const Vector3f b) { return *this = *this + b; }
    constexpr Vector3f operator*(const float n) const { return {x * n, y * n, z * n}; }
    constexpr Vector3f operator/(const float n) const { return *this * (1.0f / n); }

    constexpr float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    inline constexpr Vector3f cross(const Vector3f b) const {
        return {this->y * b.z - this->z * b.y, this->z * b.x - this->x * b.z, this->x * b.y - this->y * b.x};
    }

    constexpr float square() const { return this->dot(*this); }
    float length() const { return std::sqrt(square()); }
    Vector3f normalize() const {
        float inv_sqrt = 1.0f / std::sqrt(this->square());
        return *this * inv_sqrt;
    }
};

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
};

struct Quaternion {
    float x, y, z, w;
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    static constexpr Quaternion identity() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1，顺时针旋转（沿着轴看过去）
    static Quaternion from_rotation(Vector3f axis, float angle) {
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度。实际计算顺序和逻辑上的应用顺序相反
    static Quaternion from_eular(Vector3f rotate) {
        return Quaternion::from_rotation({1, 0, 0}, rotate.x) * Quaternion::from_rotation({0, 1, 0}, rotate.y) *
               Quaternion::from_rotation({0, 0, 1}, rotate.z);
    }

    Vector3f rotate_direction(Vector3f src) const {
        const Quaternion &q = *this;
        Quaternion p{src.x, src.y, src.z, 0.0f};
        Quaternion rotated = q * p * q.conjugate();
        return Vector3f{rotated.x, rotated.y, rotated.z}.normalize();
    }

    float length() const noexcept { return std::sqrtf(x * x + y * y + z * z + w * w); }

    Quaternion normalize() const noexcept {
        float s = 1.0f / length();
        return Quaternion{x * s, y * s, z * s, w * s};
    }

    constexpr Quaternion conjugate() const { return Quaternion{-x, -y, -z, w}; }

    Quaternion operator*(const Quaternion r) const noexcept {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;
        // 每次计算后归一化防止浮点误差累积
        return Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)}.normalize();
    }

    Quaternion operator*=(const Quaternion r) noexcept {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;
        // 每次计算后归一化防止浮点误差累积
        *this = Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)}.normalize();
        return *this;
    }
};

struct Matrix3 {
    float m[3][3];

    constexpr Matrix3() : m{{0}} {}
    constexpr Matrix3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
        : m{{m00, m01, m02}, {m10, m11, m12}, {m20, m21, m22}} {}
    static constexpr Matrix3 identity() { return Matrix3{1, 0, 0, 0, 1, 0, 0, 0, 1}; }
    static constexpr Matrix3 zero() { return Matrix3{}; }
    const float *data() const { return *m; }

    constexpr Matrix3 transpose() const {
        return Matrix3{
            m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2],
        };
    }

    constexpr Matrix3 operator*(const Matrix3 &m) const {
        Matrix3 r;
        for (unsigned int i = 0; i < 3; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j];
            }
        }
        return r;
    }

    constexpr Vector3f operator*(const Vector3f &right) const {
        Vector3f r;
        r.v[0] = m[0][0] * right.v[0] + m[0][1] * right.v[1] + m[0][2] * right.v[2];
        r.v[1] = m[1][0] * right.v[0] + m[1][1] * right.v[1] + m[1][2] * right.v[2];
        r.v[2] = m[2][0] * right.v[0] + m[2][1] * right.v[1] + m[2][2] * right.v[2];
        return r;
    }

    // ----------------构造缩放，旋转矩阵--------------------
    static constexpr Matrix3 scale(float x, float y, float z) { return Matrix3{x, 0, 0, 0, y, 0, 0, 0, z}; }
    static constexpr Matrix3 scale(Vector3f scale) { return Matrix3::scale(scale.x, scale.y, scale.z); }

    static constexpr Matrix3 rotate(Quaternion rotation) {
        auto [x, y, z, w] = rotation;
        return Matrix3{
            1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y + w * z),        2.0f * (x * z - w * y),
            2.0f * (x * y - w * z),        1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + w * x),
            2.0f * (x * z + w * y),        2.0f * (y * z - w * x),        1.0f - 2.0f * (x * x + y * y),
        };
    }

    // ----------------从矩阵中提取缩放，旋转--------------------
    constexpr Vector3f resolve_scale() const noexcept {
        // 乘在右边的缩放矩阵对矩阵的每个列向量进行了缩放
        Vector3f c1 = {m[0][0], m[0][1], m[0][2]};
        Vector3f c2 = {m[1][0], m[1][1], m[1][2]};
        Vector3f c3 = {m[2][0], m[2][1], m[2][2]};
        return {c1.length(), c2.length(), c3.length()};
    }
    // 从矩阵中反解出其旋转对应的四元数
    Quaternion resolve_rotation() const noexcept {
        // 为了数值稳定性，使用这个包含4个开方的版本
        // 还有另一种版本基于比较+使用最大数的版本
        float qx = 0.5f * std::sqrt(m[0][0] - m[1][1] - m[2][2]);
        float qy = 0.5f * std::sqrt(-m[0][0] + m[1][1] - m[2][2]);
        float qz = 0.5f * std::sqrt(-m[0][0] - m[1][1] + m[2][2]);
        float qw = 0.5f * std::sqrt(m[0][0] + m[1][1] + m[2][2]);
        return Quaternion{qx, qy, qz, qw};
    }
};

struct Matrix4 {
    float m[4][4]; // 以行主序存储矩阵

    constexpr Matrix4() : m{{0}} {}
    constexpr Matrix4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                      float m21, float m22, float m23, float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13}, {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}
    static constexpr Matrix4 identity() {
        return Matrix4{
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
        };
    }
    static constexpr Matrix4 zero() { return Matrix4{}; }
    explicit constexpr Matrix4(const Matrix3 &mat, float m44 = 1.0f)
        : m{{mat.m[0][0], mat.m[0][1], mat.m[0][2], 0.0},
            {mat.m[1][0], mat.m[1][1], mat.m[1][2], 0.0},
            {mat.m[2][0], mat.m[2][1], mat.m[2][2], 0.0},
            {0.0, 0.0, 0.0, m44}} {}

    const float *data() const { return *m; }

    constexpr Matrix4 transpose() const {
        return Matrix4{m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1],
                       m[0][2], m[1][2], m[2][2], m[3][2], m[0][3], m[1][3], m[2][3], m[3][3]};
    }

    constexpr Matrix4 operator*(const Matrix4 &m) const {
        Matrix4 r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j] +
                            this->m[i][3] * m.m[3][j];
            }
        }
        return r;
    }

    constexpr Vector4f operator*(const Vector4f &right) const {
        Vector4f r;
        r.v[0] = m[0][0] * right.v[0] + m[0][1] * right.v[1] + m[0][2] * right.v[2] + m[0][3] * right.v[3];
        r.v[1] = m[1][0] * right.v[0] + m[1][1] * right.v[1] + m[1][2] * right.v[2] + m[1][3] * right.v[3];
        r.v[2] = m[2][0] * right.v[0] + m[2][1] * right.v[1] + m[2][2] * right.v[2] + m[2][3] * right.v[3];
        r.v[3] = m[3][0] * right.v[0] + m[3][1] * right.v[1] + m[3][2] * right.v[2] + m[3][3] * right.v[3];
        return r;
    }
    // ----------------构造缩放，旋转，位移矩阵--------------------
    static constexpr Matrix4 translate(float x, float y, float z) {
        return Matrix4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};
    }
    static constexpr Matrix4 translate(Vector3f delta) { return Matrix4::translate(delta.x, delta.y, delta.z); }
    static constexpr Matrix4 rotate(Quaternion rotation) { return Matrix4{Matrix3::rotate(rotation)}; }
    static constexpr Matrix4 scale(float x, float y, float z) {
        return Matrix4{
            x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0,
        };
    }
    static constexpr Matrix4 scale(Vector3f scale) { return Matrix4::scale(scale.x, scale.y, scale.z); }

    // ----------------从矩阵中提取缩放，旋转，位移--------------------
    constexpr Vector3f resolve_scale() const noexcept {
        // 乘在右边的缩放矩阵对矩阵的每个列向量进行了缩放
        Vector3f c1 = Vector3f{m[0][0], m[0][1], m[0][2]};
        Vector3f c2 = Vector3f{m[1][0], m[1][1], m[1][2]};
        Vector3f c3 = Vector3f{m[2][0], m[2][1], m[2][2]};
        return Vector3f{c1.length(), c2.length(), c3.length()} / m[3][3];
    }

    constexpr Vector3f resolve_translate() const noexcept { return Vector3f{m[3][0], m[3][1], m[3][2]} / m[3][3]; }

    // 从矩阵中反解出其旋转对应的四元数
    Quaternion resolve_rotation() const noexcept {
        // 为了数值稳定性，使用这个包含4个开方的版本
        // 还有另一种版本基于比较+使用最大数的版本
        float qx = 0.5f * std::sqrt(m[0][0] - m[1][1] - m[2][2] + m[3][3]);
        float qy = 0.5f * std::sqrt(-m[0][0] + m[1][1] - m[2][2] + m[3][3]);
        float qz = 0.5f * std::sqrt(-m[0][0] - m[1][1] + m[2][2] + m[3][3]);
        float qw = 0.5f * std::sqrt(m[0][0] + m[1][1] + m[2][2] + m[3][3]);
        return Quaternion{qx, qy, qz, qw};
    }
};

struct Transform {
    Vector3f position;
    Quaternion rotation;
    Vector3f scale;

    constexpr explicit Transform(Vector3f position = {0, 0, 0}, Quaternion rotation = Quaternion::identity(),
                        Vector3f scale = {1, 1, 1})
        : position(position), rotation(rotation), scale(scale) {}
    static constexpr Transform from_matrix(const Matrix4 &matrix) {
        return Transform{matrix.resolve_translate(), matrix.resolve_rotation(), matrix.resolve_scale()};
    }
    static constexpr Transform from_matrix(const Matrix3 &matrix) {
        return Transform{{}, matrix.resolve_rotation(), matrix.resolve_scale()};
    }

    Vector3f get_forward_direction() const { return rotation.rotate_direction({0, 0, -1}); }

    Vector3f get_up_direction() const { return rotation.rotate_direction({0, 1, 0}); }

    constexpr Matrix4 model_matrix() const {
        return Matrix4{Matrix3::scale(scale) * Matrix3::rotate(rotation)} * Matrix4::translate(position);
    }
    constexpr Matrix3 normal_matrix() const {
        // 旋转矩阵 * 缩放矩阵的伴随矩阵
        return Matrix3::scale(scale.y * scale.z, scale.x * scale.z, scale.y * scale.z) * Matrix3::rotate(rotation);
    }
};

struct BoundingBox {
    Vector3f min;
    Vector3f max;

    constexpr BoundingBox() noexcept = default;
    constexpr BoundingBox(Vector3f min, Vector3f max) noexcept : min(min), max(max) {
        assert(min.x <= max.x && min.y <= max.y && min.z <= max.z);
    }

    constexpr BoundingBox offset(Vector3f pos) const noexcept { return BoundingBox{min + pos, max + pos}; }

    constexpr bool contains(Vector3f pos) const noexcept {
        return pos.x >= min.x && pos.y >= min.y && pos.z >= min.z && pos.x <= max.x && pos.y <= max.y && pos.z <= max.z;
    }
    constexpr float volume() const noexcept {
        Vector3f vec = max - min;
        return vec.x * vec.y * vec.z;
    }
    constexpr Vector3f center() const noexcept { return (min + max) * 0.5; }
};

} // namespace Goonya

template <>
struct std::formatter<Goonya::Vector2f> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Vector2f vec2, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {})", vec2.x, vec2.y);
    }
};

template <>
struct std::formatter<Goonya::Vector3f> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Vector3f vec3, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", vec3.x, vec3.y, vec3.z);
    }
};