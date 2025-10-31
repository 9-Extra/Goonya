#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <type_traits>

namespace Goonya {

constexpr float to_radian(float angle) { return angle / 180.0f * 3.1415926535f; }

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

    constexpr Vector3f operator+(const Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }
    constexpr Vector3f operator-(const Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }
    constexpr Vector3f operator-() const { return Vector3f{-x, -y, -z}; }
    constexpr Vector3f operator+=(const Vector3f b) { return *this = *this + b; }
    constexpr Vector3f operator*(const float n) const { return {x * n, y * n, z * n}; }
    constexpr Vector3f operator/(const float n) const { return *this * (1.0f / n); }
    constexpr Vector3f operator*(const Vector3f v) const { return {x * v.x, y * v.y, z * v.z}; }

    friend constexpr Vector3f operator*(Vector3f left, Matrix3 right) noexcept;
    constexpr float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    constexpr Vector3f cross(const Vector3f b) const {
        return {this->y * b.z - this->z * b.y, this->z * b.x - this->x * b.z, this->x * b.y - this->y * b.x};
    }
    inline constexpr Vector3f apply(Quaternion q) const noexcept;

    constexpr float square() const { return this->dot(*this); }
    float length() const { return std::sqrt(square()); }
    Vector3f normalize() const {
        float inv_sqrt = 1.0f / std::sqrt(this->square());
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

struct Quaternion {
    float x, y, z, w;
    constexpr Quaternion() : x(0), y(0), z(0), w(1) {}
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    static constexpr Quaternion identity() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1，顺时针旋转（沿着轴看过去）
    static Quaternion from_rotation(Vector3f axis, float angle) {
        assert(axis.length() == 1);
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度
    static Quaternion from_eular(Vector3f rotate) {
        return Quaternion::from_rotation({1, 0, 0}, rotate.x) * Quaternion::from_rotation({0, 1, 0}, rotate.y) *
               Quaternion::from_rotation({0, 0, 1}, rotate.z);
    }

    constexpr Vector3f apply(Vector3f src) const noexcept {
        // from godot: https://github.com/godotengine/godot
        Vector3f u{x, y, z};
        Vector3f uv = u.cross(src);
        return src + ((uv * w) + u.cross(uv)) * 2.0f;
    }

    constexpr Vector3f forward_direction() const noexcept { return apply(FORWARD); }

    constexpr Vector3f up_direction() const noexcept { return apply(UP); }
    float length() const noexcept { return std::sqrtf(x * x + y * y + z * z + w * w); }

    Quaternion normalize() const noexcept {
        float s = 1.0f / length();
        return Quaternion{x * s, y * s, z * s, w * s};
    }

    constexpr Quaternion conjugate() const { return Quaternion{-x, -y, -z, w}; }

    Quaternion operator*(const Quaternion r) const noexcept {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;
        return Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)};
    }

    Quaternion operator*=(const Quaternion r) noexcept {
        *this = *this * r;
        return *this;
    }

    bool operator==(this const Quaternion &self, const Quaternion r) noexcept {
        return self.x == r.x && self.y == r.y && self.z == r.z && self.w == r.w;
    }
};

constexpr Vector3f Vector3f::apply(Quaternion q) const noexcept{
    return q.apply(*this);
}

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
        float qx = (m[2][1] < m[1][2] ? 0.5f : -0.5f) * std::sqrt(m[0][0] - m[1][1] - m[2][2] + 1);
        float qy = (m[0][2] < m[2][0] ? 0.5f : -0.5f) * std::sqrt(-m[0][0] + m[1][1] - m[2][2] + 1);
        float qz = (m[1][0] < m[0][1] ? 0.5f : -0.5f) * std::sqrt(-m[0][0] - m[1][1] + m[2][2] + 1);
        float qw = 0.5f * std::sqrt(m[0][0] + m[1][1] + m[2][2] + 1);
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

    constexpr float &operator[](size_t i, size_t j) {
        assert(i < 4 && j < 4);
        return m[i][j];
    }

    /**
     * @brief 取矩阵的第i行
     */
    constexpr Vector4f &operator[](size_t i) noexcept {
        static_assert(sizeof(Matrix4) == sizeof(Vector4f[4]), "意想不到的内存布局");
        assert(i < 4);
        return reinterpret_cast<Vector4f &>(m[i]);
    }

    constexpr Matrix4 operator*(const Matrix4 &m) const noexcept {
        Matrix4 r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j] +
                            this->m[i][3] * m.m[3][j];
            }
        }
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

    constexpr Vector3f resolve_position() const noexcept { return Vector3f{m[3][0], m[3][1], m[3][2]} / m[3][3]; }

    // 从矩阵中反解出其旋转对应的四元数
    Quaternion resolve_rotation() const noexcept {
        // 为了数值稳定性，使用这个包含4个开方的版本
        // 还有另一种版本基于比较+使用最大数的版本
        float qx = (m[2][1] < m[1][2] ? 0.5f : -0.5f) * std::sqrt(m[0][0] - m[1][1] - m[2][2] + m[3][3]);
        float qy = (m[0][2] < m[2][0] ? 0.5f : -0.5f) * std::sqrt(-m[0][0] + m[1][1] - m[2][2] + m[3][3]);
        float qz = (m[1][0] < m[0][1] ? 0.5f : -0.5f) * std::sqrt(-m[0][0] - m[1][1] + m[2][2] + m[3][3]);
        float qw = 0.5f * std::sqrt(m[0][0] + m[1][1] + m[2][2] + m[3][3]);
        return Quaternion{qx, qy, qz, qw};
    }

    // --------------------------------------------
    Matrix3 to_matrix3() const noexcept {
        return Matrix3{
            m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2],
        };
    }
};

constexpr Vector3f operator*(Vector3f left, Matrix3 right) noexcept {
    Vector3f r;
    // 将向量视为行向量，左乘矩阵
    r.v[0] = left[0] * right.m[0][0] + left[1] * right.m[1][0] + left[2] * right.m[2][0];
    r.v[1] = left[0] * right.m[0][1] + left[1] * right.m[1][1] + left[2] * right.m[2][1];
    r.v[2] = left[0] * right.m[0][2] + left[1] * right.m[1][2] + left[2] * right.m[2][2];
    return r;
}

constexpr Vector4f operator*(Vector4f left, Matrix4 right) noexcept {
    Vector4f r;
    // 将向量视为行向量，左乘矩阵
    r.v[0] = left[0] * right.m[0][0] + left[1] * right.m[1][0] + left[2] * right.m[2][0] + left[3] * right.m[3][0];
    r.v[1] = left[0] * right.m[0][1] + left[1] * right.m[1][1] + left[2] * right.m[2][1] + left[3] * right.m[3][1];
    r.v[2] = left[0] * right.m[0][2] + left[1] * right.m[1][2] + left[2] * right.m[2][2] + left[3] * right.m[3][2];
    r.v[3] = left[0] * right.m[0][3] + left[1] * right.m[1][3] + left[2] * right.m[2][3] + left[3] * right.m[3][3];
    return r;
}

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

struct BoundingBox {
    Vector3f min;
    Vector3f max;

    constexpr static BoundingBox infinite() {
        // 不使用浮点inf作为包围盒长度，因为计算center会得到nan
        float max = std::numeric_limits<float>::max();
        float min = std::numeric_limits<float>::min();
        return BoundingBox{{-min, -min, -min}, {max, max, max}};
    }

    constexpr BoundingBox() noexcept : min{0, 0, 0}, max{0, 0, 0} {};
    constexpr BoundingBox(Vector3f min, Vector3f max) noexcept : min(min), max(max) {
        assert(min.x <= max.x && min.y <= max.y && min.z <= max.z);
    }

    constexpr BoundingBox offset(Vector3f pos) const noexcept { return BoundingBox{min + pos, max + pos}; }
    /**
     * @brief 获取变换后后的保守包围盒
     */
    constexpr BoundingBox transformed(const Matrix4 &transform) const noexcept {
        /*
        此函数源自 [Godot] (https://github.com/godotengine/godot)
        版权归 [Juan Linietsky, Ariel Manzur] 所有
        遵循 MIT 许可证
        */
        Matrix3 rotation_scale = transform.to_matrix3().transpose();
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

template <>
struct std::formatter<Goonya::Quaternion> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Quaternion q, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", q.x, q.y, q.z, q.w);
    }
};