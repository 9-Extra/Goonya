#pragma once

#include <cassert>
#include <cmath>
#include <format>

namespace Goonya {

inline float to_radian(float angle) { return angle / 180.0f * 3.1415926535f; }

struct Vector2f {
    union {
        struct {
            float x, y;
        };
        float v[2];
    };
    Vector2f() {}
    constexpr Vector2f(float x, float y) : x(x), y(y) {}

    Vector2f operator+(const Vector2f b) { return Vector2f(x + b.x, y + b.y); }

    Vector2f operator-(const Vector2f b) { return Vector2f(x - b.x, y - b.y); }

    Vector2f operator*(const float s) { return Vector2f(x * s, y * s); }

    float squared() { return x * x + y * y; }

    float length() { return sqrtf(squared()); }

    Vector2f normalized() {
        float s = 1.0f / std::sqrt(squared());
        return Vector2f(x * s, y * s);
    }

    Vector2f rotate(float radiam) {
        return Vector2f(x * cosf(radiam) + y * sinf(radiam), x * -sinf(radiam) + y * cosf(radiam));
    }
};

struct Color {
    float r, g, b, a = 1.0f;

    inline const float *data() const { return (float *)this; }

    inline bool operator==(const Color &ps) { return r == ps.r && g == ps.g && b == ps.b && a == ps.a; }

    inline bool operator!=(const Color &ps) { return !(*this == ps); }
};

struct Vector3f {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };

    constexpr Vector3f() : x(0), y(0), z(0) {}
    constexpr Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}

    const float *data() const { return (float *)v; }

    inline float operator[](const unsigned int i) const { return v[i]; }

    inline Vector3f operator+(const Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }

    inline Vector3f operator-(const Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }

    inline Vector3f operator-() const { return Vector3f{-x, -y, -z}; }

    inline Vector3f operator+=(const Vector3f b) { return *this = *this + b; }

    inline Vector3f operator*(const float n) { return {x * n, y * n, z * n}; }
    inline Vector3f operator/(const float n) { return *this * (1.0f / n); }

    inline float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    inline constexpr Vector3f cross(const Vector3f b) {
        return {this->y * b.z - this->z * b.y, this->z * b.x - this->x * b.z, this->x * b.y - this->y * b.x};
    }

    inline float square() const { return this->dot(*this); }

    inline Vector3f normalize() {
        float inv_sqrt = 1.0f / std::sqrt(this->square());
        return *this * inv_sqrt;
    }

    inline float length() const { return std::sqrt(square()); }
};

struct Vector4f {
    union {
        struct {
            float x, y, z, w;
        };
        float v[4];
    };

    constexpr Vector4f() : x(0), y(0), z(0), w(0) {}
    constexpr Vector4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    constexpr Vector4f(Vector3f vec3, float w) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {}

    constexpr Vector3f get_xyz() const noexcept { return Vector3f{x, y, z}; }
};

struct Matrix3 {
    float m[3][3];

    Matrix3() {}
    constexpr Matrix3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
        : m{{m00, m01, m02}, {m10, m11, m12}, {m20, m21, m22}} {}

    const float *data() const { return (float *)m; }

    inline Matrix3 transpose() const {
        Matrix3 r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[j][i];
            }
        }
        return r;
    }

    constexpr static Matrix3 identity() {
        Matrix3 m{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

        return m;
    }

    inline Matrix3 operator*(const Matrix3 &m) const {
        Matrix3 r;
        for (unsigned int i = 0; i < 3; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j];
            }
        }
        return r;
    }

    inline Vector3f operator*(const Vector3f &right) const {
        Vector3f r;
        r.v[0] = m[0][0] * right.v[0] + m[0][1] * right.v[1] + m[0][2] * right.v[2];
        r.v[1] = m[1][0] * right.v[0] + m[1][1] * right.v[1] + m[1][2] * right.v[2];
        r.v[2] = m[2][0] * right.v[0] + m[2][1] * right.v[1] + m[2][2] * right.v[2];
        return r;
    }

    inline static Matrix3 scale(float x, float y, float z) { return Matrix3{x, 0, 0, 0, y, 0, 0, 0, z}; }

    inline static Matrix3 scale(Vector3f scale) { return Matrix3::scale(scale.x, scale.y, scale.z); }
};

struct Matrix4 {
    float m[4][4]; // 以行主序存储矩阵

    Matrix4() {}
    constexpr Matrix4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                      float m21, float m22, float m23, float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13}, {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}
    explicit constexpr Matrix4(const Matrix3 &mat, float m44 = 1.0f)
        : m{{mat.m[0][0], mat.m[0][1], mat.m[0][2], 0.0},
            {mat.m[1][0], mat.m[1][1], mat.m[1][2], 0.0},
            {mat.m[2][0], mat.m[2][1], mat.m[2][2], 0.0},
            {0.0, 0.0, 0.0, m44}} {}

    const float *data() const { return (float *)m; }

    inline Matrix4 transpose() const {
        Matrix4 r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[j][i];
            }
        }
        return r;
    }

    constexpr static Matrix4 identity() {
        Matrix4 m{
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
        };

        return m;
    }

    inline Matrix4 operator*(const Matrix4 &m) const {
        Matrix4 r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j] +
                            this->m[i][3] * m.m[3][j];
            }
        }
        return r;
    }

    inline Vector4f operator*(const Vector4f &right) const {
        Vector4f r;
        r.v[0] = m[0][0] * right.v[0] + m[0][1] * right.v[1] + m[0][2] * right.v[2] + m[0][3] * right.v[3];
        r.v[1] = m[1][0] * right.v[0] + m[1][1] * right.v[1] + m[1][2] * right.v[2] + m[1][3] * right.v[3];
        r.v[2] = m[2][0] * right.v[0] + m[2][1] * right.v[1] + m[2][2] * right.v[2] + m[2][3] * right.v[3];
        r.v[3] = m[3][0] * right.v[0] + m[3][1] * right.v[1] + m[3][2] * right.v[2] + m[3][3] * right.v[3];
        return r;
    }

    inline static Matrix4 translate(float x, float y, float z) {
        Matrix4 m{
            1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0,
        };
        return m;
    }

    inline static Matrix4 translate(Vector3f delta) { return Matrix4::translate(delta.x, delta.y, delta.z); }

    inline static Vector3f parse_translate(const Matrix4 &m) {
        return Vector3f{m.m[0][3], m.m[1][3], m.m[2][3]} / m.m[3][3];
    }

    inline static Matrix4 scale(float x, float y, float z) {
        Matrix4 m{
            x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0,
        };

        return m;
    }

    inline static Matrix4 scale(Vector3f scale) { return Matrix4::scale(scale.x, scale.y, scale.z); }
};

struct Quaternion {
    float x, y, z, w;
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    static constexpr Quaternion indentity() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1，顺时针旋转（沿着轴看过去）
    static constexpr Quaternion from_rotation(Vector3f axis, float angle) {
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度。实际计算顺序和逻辑上的应用顺序相反
    static constexpr Quaternion from_eular(Vector3f rotate) {
        return Quaternion::from_rotation({0, 0, 1}, rotate.z) * Quaternion::from_rotation({0, 1, 0}, rotate.y) *
               Quaternion::from_rotation({1, 0, 0}, rotate.x);
    }

    // 从矩阵中反解出其旋转对应的四元数
    static constexpr Quaternion from_matrix(Matrix3 m) {
        // 为了数值稳定性，使用这个包含4个开方的版本
        // 还有另一种版本基于比较+使用最大数的版本
        float qx = 0.5f * std::sqrt(m.m[0][0] - m.m[1][1] - m.m[2][2]);
        float qy = 0.5f * std::sqrt(-m.m[0][0] + m.m[1][1] - m.m[2][2]);
        float qz = 0.5f * std::sqrt(-m.m[0][0] - m.m[1][1] + m.m[2][2]);
        float qw = 0.5f * std::sqrt(m.m[0][0] + m.m[1][1] + m.m[2][2]);
        return Quaternion{qx, qy, qz, qw};
    }

    // 从矩阵中反解出其旋转对应的四元数
    static constexpr Quaternion from_matrix(Matrix4 m) {
        float qx = 0.5f * std::sqrt(m.m[0][0] - m.m[1][1] - m.m[2][2] + m.m[3][3]);
        float qy = 0.5f * std::sqrt(-m.m[0][0] + m.m[1][1] - m.m[2][2] + m.m[3][3]);
        float qz = 0.5f * std::sqrt(-m.m[0][0] - m.m[1][1] + m.m[2][2] + m.m[3][3]);
        float qw = 0.5f * std::sqrt(m.m[0][0] + m.m[1][1] + m.m[2][2] + m.m[3][3]);
        return Quaternion{qx, qy, qz, qw};
    }

    // 需要长度为1
    constexpr Matrix3 to_matrix() const {
        return Matrix3{
            1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z),        2.0f * (x * z + w * y),
            2.0f * (x * y + w * z),        1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x),
            2.0f * (x * z - w * y),        2.0f * (y * z + w * x),        1.0f - 2.0f * (x * x + y * y),
        };
    }

    constexpr Vector3f rotate_direction(Vector3f src) const {
        const Quaternion &q = *this;
        Quaternion p{src.x, src.y, src.z, 0.0f};
        Quaternion rotated = q * p * q.conjugate();
        return Vector3f{rotated.x, rotated.y, rotated.z}.normalize();
    }

    constexpr float length() const noexcept { return std::sqrtf(x * x + y * y + z * z + w * w); }

    constexpr Quaternion normalize() const noexcept {
        float s = 1.0f / length();
        return Quaternion{x * s, y * s, z * s, w * s};
    }

    constexpr Quaternion conjugate() const { return Quaternion{-x, -y, -z, w}; }

    constexpr Quaternion operator*(const Quaternion r) const noexcept {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;
        // 每次计算后归一化防止浮点误差累积
        return Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)}.normalize();
    }

    constexpr Quaternion operator*=(const Quaternion r) noexcept {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;
        // 每次计算后归一化防止浮点误差累积
        *this = Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)}.normalize();
        return *this;
    }
};

struct Transform {
    Vector3f position;
    Quaternion rotation;
    Vector3f scale;

    constexpr Transform(Vector3f position = {0, 0, 0}, Quaternion rotation = Quaternion::indentity(),
                        Vector3f scale = {1, 1, 1})
        : position(position), rotation(rotation), scale(scale) {}
    // static Transform from_matrix(const Matrix4 &matrix) {
    //     Transform transform;
    //     transform.position = Vector3f(matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]);
    //     // transform.rotation = rotation_matrix_to_eulerangles(matrix);
    //     transform.scale = Vector3f(1, 1, 1);
    //     return transform;
    // }

    constexpr Vector3f get_forward_direction() const { return rotation.rotate_direction({0, 0, -1}); }

    constexpr Vector3f get_up_direction() const { return rotation.rotate_direction({0, 1, 0}); }

    constexpr Matrix4 model_matrix() const {
        return Matrix4::translate(position) * Matrix4{rotation.to_matrix() * Matrix3::scale(scale)};
    }
    constexpr Matrix3 normal_matrix() const {
        // 旋转矩阵 * 缩放矩阵的伴随矩阵
        return rotation.to_matrix() * Matrix3::scale(scale.y * scale.z, scale.x * scale.z, scale.y * scale.z);
    }
};

inline Vector3f position_from_matrix(const Matrix4 &matrix) {
    return Vector3f(matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]);
}

struct BoundingBox {
    Vector3f min;
    Vector3f max;

    BoundingBox() noexcept {}
    BoundingBox(Vector3f min, Vector3f max) noexcept : min(min), max(max) {
        assert(min.x <= max.x && min.y <= max.y && min.z <= max.z);
    }

    BoundingBox offset(Vector3f pos) const noexcept { return BoundingBox{min + pos, max + pos}; }

    bool contains(Vector3f pos) const noexcept {
        return pos.x >= min.x && pos.y >= min.y && pos.z >= min.z && pos.x <= max.x && pos.y <= max.y && pos.z <= max.z;
    }

    float volume() const noexcept {
        Vector3f vec = max - min;
        return vec.x * vec.y * vec.z;
    }

    Vector3f center() const noexcept { return (min + max) * 0.5; }
};

} // namespace Goonya

template <>
struct std::formatter<Goonya::Vector2f> {
    auto parse(std::format_parse_context &context) { return context.begin(); }
    auto format(const Goonya::Vector2f vec2, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "({}, {})", vec2.x, vec2.y);
    }
};

template <>
struct std::formatter<Goonya::Vector3f> {
    auto parse(std::format_parse_context &context) { return context.begin(); }
    auto format(const Goonya::Vector3f vec3, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", vec3.x, vec3.y, vec3.z);
    }
};