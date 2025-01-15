#pragma once

#include <cassert>
#include <cmath>
#include <iostream>

namespace Goonya {

static float Q_rsqrt(float number) {
    const float threehalfs = 1.5F;

    float x2 = number * 0.5F;
    float y = number;
    int32_t i = *(int32_t *)&y; // evil floating point bit level hacking
    i = 0x5f3759df - (i >> 1);  // what the fuck?
    y = *(float *)&i;
    y = y * (threehalfs - (x2 * y * y)); // 1st iteration
    // y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be
    // removed

    return y;
}

inline float to_radian(float angle) { return angle / 180.0f * 3.1415926535f; }

struct Vector2f {
    union {
        struct {
            float x, y;
        };
        float v[2];
    };
    Vector2f() : x(0), y(0) {}
    Vector2f(float x, float y) : x(x), y(y) {}

    Vector2f operator+(const Vector2f b) { return Vector2f(x + b.x, y + b.y); }

    Vector2f operator-(const Vector2f b) { return Vector2f(x - b.x, y - b.y); }

    Vector2f operator*(const float s) { return Vector2f(x * s, y * s); }

    float squared() { return x * x + y * y; }

    float length() { return sqrtf(squared()); }

    Vector2f normalized() {
        float s = Q_rsqrt(squared());
        return Vector2f(x * s, y * s);
    }

    Vector2f rotate(float radiam) {
        return Vector2f(x * cosf(radiam) + y * sinf(radiam), x * -sinf(radiam) + y * cosf(radiam));
    }

    friend std::ostream &operator<<(std::ostream &os, const Vector2f &v) {
        os << "(" << v.x << ", " << v.y << ")";
        return os;
    }
};

struct Color {
    float r, g, b;

    inline const float *data() const { return (float *)this; }

    inline bool operator==(const Color &ps) { return r == ps.r && g == ps.g && b == ps.b; }

    inline bool operator!=(const Color &ps) { return !(*this == ps); }
};

struct Vector3f {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };

    Vector3f() {}
    Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}

    const float *data() const { return (float *)v; }

    inline float operator[](const unsigned int i) const { return v[i]; }

    inline Vector3f operator+(const Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }

    inline Vector3f operator-(const Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }

    inline Vector3f operator-() const { return Vector3f{-x, -y, -z}; }

    inline Vector3f operator+=(const Vector3f b) { return *this = *this + b; }

    inline Vector3f operator*(const float n) { return {x * n, y * n, z * n}; }
    inline Vector3f operator/(const float n) { return *this * (1.0f / n); }

    inline float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    inline Vector3f cross(const Vector3f b) {
        return {this->y * b.z - this->z * b.y, this->z * b.x * this->x * b.z, this->x * b.y - this->y * b.x};
    }

    inline float square() const { return this->dot(*this); }

    inline Vector3f normalize() {
        float inv_sqrt = Q_rsqrt(this->square());
        return *this * inv_sqrt;
    }

    inline float length() const { return std::sqrt(square()); }

    friend std::ostream &operator<<(std::ostream &os, const Vector3f &v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

struct Vector4f {
    union {
        struct {
            float x, y, z, w;
        };
        float v[4];
    };
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
    // 沿z轴顺时针旋转roll，沿x轴顺时针旋转pitch，沿y轴顺时针旋转yaw
    inline static Matrix3 rotate(float roll, float pitch, float yaw) {
        float s_p = sin(pitch), c_p = cos(pitch);
        float s_r = sin(roll), c_r = cos(roll);
        float s_y = sin(yaw), c_y = cos(yaw);
        Matrix3 m{
            -s_p * s_r * s_y + c_r * c_y, -s_p * s_y * c_r - s_r * c_y, -s_y * c_p, s_r * c_p, c_p * c_r, -s_p,
            s_p * s_r * c_y + s_y * c_r,  s_p * c_r * c_y - s_r * s_y,  c_p * c_y,
        };
        return m;
    }

    inline static Matrix3 rotate(Vector3f rotate) { return Matrix3::rotate(rotate.x, rotate.y, rotate.z); }

    inline static Matrix3 scale(float x, float y, float z) {
        Matrix3 m{x, 0.0, 0.0, y, 0.0, 0.0, z, 0.0, 0.0};

        return m;
    }

    inline static Matrix3 scale(Vector3f scale) { return Matrix3::scale(scale.x, scale.y, scale.z); }
};

struct Matrix4 {
    float m[4][4];

    Matrix4() {}
    constexpr Matrix4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                      float m21, float m22, float m23, float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13}, {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}
    Matrix4(const Matrix3 &mat, float m44 = 1.0f)
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
    // 沿z轴顺时针旋转roll，沿x轴顺时针旋转pitch，沿y轴顺时针旋转yaw
    inline static Matrix4 rotate(float roll, float pitch, float yaw) {
        float s_p = sin(pitch), c_p = cos(pitch);
        float s_r = sin(roll), c_r = cos(roll);
        float s_y = sin(yaw), c_y = cos(yaw);
        Matrix4 m{-s_p * s_r * s_y + c_r * c_y,
                  -s_p * s_y * c_r - s_r * c_y,
                  -s_y * c_p,
                  0.0,
                  s_r * c_p,
                  c_p * c_r,
                  -s_p,
                  0.0,
                  s_p * s_r * c_y + s_y * c_r,
                  s_p * c_r * c_y - s_r * s_y,
                  c_p * c_y,
                  0.0,
                  0.0,
                  0.0,
                  0.0,
                  1.0};
        return m;
    }

    inline static Matrix4 rotate(Vector3f rotate) { return Matrix4::rotate(rotate.x, rotate.y, rotate.z); }

    inline static Matrix4 translate(float x, float y, float z) {
        Matrix4 m{
            1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0,
        };
        return m;
    }

    inline static Matrix4 translate(Vector3f delta) { return Matrix4::translate(delta.x, delta.y, delta.z); }

    inline static Matrix4 scale(float x, float y, float z) {
        Matrix4 m{
            x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0,
        };

        return m;
    }

    inline static Matrix4 scale(Vector3f scale) { return Matrix4::scale(scale.x, scale.y, scale.z); }
};

inline Matrix4 compute_perspective_matrix(float ratio, float fov, float near_z, float far_z) {
    assert(near_z < far_z); // 不要写反了！！！！！！！！！！
    float SinFov = std::sin(fov * 0.5f);
    float CosFov = std::cos(fov * 0.5f);

    float Height = CosFov / SinFov;
    float Width = Height / ratio;

    return Matrix4{Width,
                   0.0f,
                   0.0f,
                   0.0f,
                   0.0f,
                   Height,
                   0.0f,
                   0.0f,
                   0.0f,
                   0.0f,
                   -far_z / (far_z - near_z),
                   -1.0f,
                   0.0f,
                   0.0f,
                   -far_z * near_z / (far_z - near_z),
                   0.0f}
        .transpose();
}

struct Quaternion {
    float x, y, z, w;

    static constexpr Quaternion no_rotate() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1
    static Quaternion from_rotation(Vector3f axis, float angle) {
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度
    static Quaternion from_eular(Vector3f rotate) {
        return Quaternion::from_rotation({1.0f, 0.0f, 0.0f}, rotate.x) *
               Quaternion::from_rotation({0.0f, 1.0f, 0.0f}, rotate.y) *
               Quaternion::from_rotation({0.0f, 0.0f, 1.0f}, rotate.z);
    }

    // 需要长度为1
    Matrix4 to_matrix() const {
        return Matrix4{1.0f - 2.0f * (y * y + z * z),
                       2.0f * (x * y - w * z),
                       2.0f * (x * z + w * y),
                       0.0f,
                       2.0f * (x * y + w * z),
                       1.0f - 2.0f * (x * x + z * z),
                       2.0f * (y * z - w * x),
                       0.0f,
                       2.0f * (x * z - w * y),
                       2.0f * (y * z + w * x),
                       1.0f - 2.0f * (x * x + y * y),
                       0.0f,
                       0.0f,
                       0.0f,
                       0.0f,
                       1.0f};
    }

    Vector3f rotate_vector(Vector3f src) const {
        const Quaternion &q = *this;
        Quaternion p{src.x, src.y, src.z, 1.0f};
        Quaternion rotated = q * p * q.conjugate();
        return Vector3f{rotated.x, rotated.y, rotated.z};
    }

    Quaternion conjugate() const { return Quaternion{-x, -y, -z, w}; }

    Quaternion operator*(const Quaternion r) const {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;

        return Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)};
    }
};

struct Transform {
    Vector3f position;
    Vector3f rotation;
    Vector3f scale;

    static Transform from_matrix(const Matrix4 &matrix) {
        Transform transform;
        transform.position = Vector3f(matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]);
        // transform.rotation = rotation_matrix_to_eulerangles(matrix);
        transform.scale = Vector3f(1, 1, 1);
        return transform;
    }

    // 获取目视方向
    Vector3f get_orientation() const {
        float pitch = rotation[1];
        float yaw = rotation[2];
        return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
    }

    Matrix4 transform_matrix() const {
        return Matrix4::translate(position) * Matrix4::rotate(rotation) * Matrix4::scale(scale);
    }
    Matrix4 normal_matrix() const {
        // 旋转矩阵 * 缩放矩阵的伴随矩阵
        return Matrix4::rotate(rotation) * Matrix4::scale({scale.y * scale.z, scale.x * scale.z, scale.y * scale.z});
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