#pragma once

#include "quaternion.h"
#include "vector.h"
#include <cassert>
#include <cstddef>
#include <optional>

namespace Goonya {

struct Matrix3f {
    float m[3][3];

    constexpr Matrix3f() : m{{0}} {}
    constexpr Matrix3f(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
        : m{{m00, m01, m02}, {m10, m11, m12}, {m20, m21, m22}} {}
    static constexpr Matrix3f identity() { return Matrix3f{1, 0, 0, 0, 1, 0, 0, 0, 1}; }
    static constexpr Matrix3f zero() { return Matrix3f{}; }
    const float *data() const { return *m; }

    constexpr Matrix3f transpose() const {
        return Matrix3f{
            m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2],
        };
    }

    constexpr Matrix3f operator*(const Matrix3f &m) const {
        Matrix3f r;
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

    constexpr bool operator==(const Matrix3f &rhs) const noexcept {
        for (unsigned int i = 0; i < 3; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                if (!is_nearly_equal(this->m[i][j], rhs.m[i][j])) {
                    return false;
                }
            }
        }
        return true;
    }

    constexpr float &operator[](size_t x, size_t y) noexcept { return m[x][y]; }

    constexpr const float &operator[](size_t x, size_t y) const noexcept { return m[x][y]; }

    constexpr float determinant() const noexcept {
        float r = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
        r -= m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]);
        r += m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
        return r;
    }

    constexpr std::optional<Matrix3f> inverse() const noexcept {
        float d = determinant();
        if (is_nearly_equal(d, 0.0f)) {
            return std::nullopt; // 不可逆
        }

        float inv_d = 1.0f / d;
        Matrix3f inv;
        for (uint32_t i = 0; i < 3; i++) {
            for (uint32_t j = 0; j < 3; j++) {
                // 每个元素换为代数余子式，转置，除以行列式
                inv.m[i][j] = ((i + j) % 2 == 0 ? 1 : -1) * cofactor(j, i) * inv_d;
            }
        }
        return inv;
    }

    static constexpr Matrix3f from_quaternion(Quaternion rotation) noexcept {
        auto [x, y, z, w] = rotation;
        return Matrix3f{
            1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y + w * z),        2.0f * (x * z - w * y),
            2.0f * (x * y - w * z),        1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + w * x),
            2.0f * (x * z + w * y),        2.0f * (y * z - w * x),        1.0f - 2.0f * (x * x + y * y),
        };
    }

    // ----------------缩放，旋转--------------------
    constexpr Matrix3f scale(float scale) const noexcept {return this->scale({scale, scale, scale});}
    constexpr Matrix3f scale(Vector3f scale) const noexcept {
        return *this * Matrix3f{scale.x, 0, 0, 0, scale.y, 0, 0, 0, scale.z};
    }

    constexpr Matrix3f rotate(Quaternion rot) const noexcept { return *this * from_quaternion(rot); }

    // ----------------从矩阵中提取缩放，旋转--------------------
    constexpr Vector3f resolve_scale() const noexcept {
        // 乘在右边的缩放矩阵对矩阵的每个列向量进行了缩放
        Vector3f c1 = {m[0][0], m[0][1], m[0][2]};
        Vector3f c2 = {m[1][0], m[1][1], m[1][2]};
        Vector3f c3 = {m[2][0], m[2][1], m[2][2]};
        return {c1.length(), c2.length(), c3.length()};
    }
    // 从矩阵中反解出其旋转对应的四元数
    Quaternion resolve_rotation_normalized() const noexcept {
        // 为了数值稳定性，使用这个包含4个开方的版本
        // 还有另一种版本基于比较+使用最大数的版本
        float qx = (m[2][1] < m[1][2] ? 0.5f : -0.5f) * std::sqrt(std::max(m[0][0] - m[1][1] - m[2][2] + 1, 0.0f));
        float qy = (m[0][2] < m[2][0] ? 0.5f : -0.5f) * std::sqrt(std::max(-m[0][0] + m[1][1] - m[2][2] + 1, 0.0f));
        float qz = (m[1][0] < m[0][1] ? 0.5f : -0.5f) * std::sqrt(std::max(-m[0][0] - m[1][1] + m[2][2] + 1, 0.0f));
        float qw = 0.5f * std::sqrt(std::max(m[0][0] + m[1][1] + m[2][2] + 1, 0.0f));
        return Quaternion{qx, qy, qz, qw};
    }

private:
    /**
     * @brief 计算余子式（不是代数余子式）
     */
    constexpr float cofactor(uint32_t x, uint32_t y) const noexcept {
        float r[2][2];
        for (uint32_t i = 0; i < 2; i++) {
            for (uint32_t j = 0; j < 2; j++) {
                r[i][j] = this->m[i >= x ? i + 1 : i][j >= y ? j + 1 : j];
            }
        }
        return r[0][0] * r[1][1] - r[1][0] * r[0][1];
    }
};

struct Matrix4f {

    float m[4][4]; // 以行主序存储矩阵

    constexpr Matrix4f() : m{{0}} {}
    constexpr Matrix4f(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                      float m21, float m22, float m23, float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13}, {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}
    static constexpr Matrix4f identity() {
        return Matrix4f{
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
        };
    }
    static constexpr Matrix4f zero() { return Matrix4f{}; }
    explicit constexpr Matrix4f(const Matrix3f &mat, float m33 = 1.0f)
        : m{{mat.m[0][0], mat.m[0][1], mat.m[0][2], 0.0},
            {mat.m[1][0], mat.m[1][1], mat.m[1][2], 0.0},
            {mat.m[2][0], mat.m[2][1], mat.m[2][2], 0.0},
            {0.0, 0.0, 0.0, m33}} {}

    const float *data() const { return *m; }

    constexpr Matrix4f transpose() const {
        return Matrix4f{m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1],
                       m[0][2], m[1][2], m[2][2], m[3][2], m[0][3], m[1][3], m[2][3], m[3][3]};
    }

    constexpr float &operator[](size_t i, size_t j) {
        assert(i < 4 && j < 4);
        return m[i][j];
    }

    /**
     * @brief 取矩阵的第i行
     */
    Vector4f &operator[](size_t i) noexcept {
        static_assert(sizeof(Matrix4f) == sizeof(Vector4f[4]), "意想不到的内存布局");
        assert(i < 4);
        return reinterpret_cast<Vector4f &>(m[i]);
    }

    constexpr Matrix4f operator*(const Matrix4f &m) const noexcept {
        Matrix4f r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j] +
                            this->m[i][3] * m.m[3][j];
            }
        }
        return r;
    }

    constexpr Matrix4f operator*(float rhs) const noexcept {
        Matrix4f r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = m[i][j] * rhs;
            }
        }
        return r;
    }

    constexpr Matrix4f operator/(float rhs) const noexcept {
        assert(!is_nearly_equal(rhs, 0.0f));
        rhs = 1 / rhs;
        Matrix4f r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = m[i][j] * rhs;
            }
        }
        return r;
    }

    constexpr bool operator==(const Matrix4f &rhs) const noexcept {
        for (uint32_t i = 0; i < 4; i++) {
            for (uint32_t j = 0; j < 4; j++) {
                if (!is_nearly_equal(this->m[i][j], rhs.m[i][j])) {
                    return false;
                }
            }
        }
        return true;
    }

    constexpr float determinant() const noexcept {
        return m[0][0] * cofactor(0, 0) - m[0][1] * cofactor(0, 1) + m[0][2] * cofactor(0, 2) -
               m[0][3] * cofactor(0, 3);
    }

    constexpr std::optional<Matrix4f> inverse() const noexcept {
        float d = determinant();
        if (is_nearly_equal(d, 0.0f)) {
            return std::nullopt; // 不可逆
        }

        float inv_d = 1.0f / d;
        Matrix4f inv;
        for (uint32_t i = 0; i < 4; i++) {
            for (uint32_t j = 0; j < 4; j++) {
                // 每个元素换为代数余子式，转置，除以行列式
                inv.m[i][j] = ((i + j) % 2 == 0 ? 1 : -1) * cofactor(j, i) * inv_d;
            }
        }
        return inv;
    }

    // ----------------缩放，旋转，位移--------------------
    constexpr Matrix4f translate(Vector3f delta) const noexcept {
        return *this * Matrix4f{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, delta.x, delta.y, delta.z, 1};
    }
    constexpr Matrix4f rotate(Quaternion rotation) const noexcept {
        return *this * Matrix4f{Matrix3f::from_quaternion(rotation)};
    }
    constexpr Matrix4f scale(float scale) const noexcept {return this->scale({scale, scale, scale});}
    constexpr Matrix4f scale(Vector3f scale) const noexcept {
        return *this * Matrix4f{scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1};
    }
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
    Quaternion resolve_rotation_normalized() const noexcept {
        // 为了数值稳定性，使用这个包含4个开方的版本
        // 还有另一种版本基于比较+使用最大数的版本
        // 同时，为了避免数值精度导致对负数开方有时发生，强行取正数
        float qx =
            (m[2][1] < m[1][2] ? 0.5f : -0.5f) * std::sqrt(std::max(m[0][0] - m[1][1] - m[2][2] + m[3][3], 0.0f));
        float qy =
            (m[0][2] < m[2][0] ? 0.5f : -0.5f) * std::sqrt(std::max(-m[0][0] + m[1][1] - m[2][2] + m[3][3], 0.0f));
        float qz =
            (m[1][0] < m[0][1] ? 0.5f : -0.5f) * std::sqrt(std::max(-m[0][0] - m[1][1] + m[2][2] + m[3][3], 0.0f));
        float qw = 0.5f * std::sqrt(std::max(m[0][0] + m[1][1] + m[2][2] + m[3][3], 0.0f));
        return Quaternion{qx, qy, qz, qw};
    }

    // --------------------------------------------
    Matrix3f to_matrix3() const noexcept {
        return Matrix3f{
            m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2],
        };
    }

private:
    /**
     * @brief 计算余子式（不是代数余子式）
     */
    constexpr float cofactor(uint32_t x, uint32_t y) const noexcept {
        Matrix3f r;
        for (uint32_t i = 0; i < 3; i++) {
            for (uint32_t j = 0; j < 3; j++) {
                r.m[i][j] = this->m[i >= x ? i + 1 : i][j >= y ? j + 1 : j];
            }
        }
        return r.determinant();
    }
};

constexpr Vector3f operator*(Vector3f left, Matrix3f right) noexcept {
    Vector3f r;
    // 将向量视为行向量，左乘矩阵
    r.v[0] = left[0] * right.m[0][0] + left[1] * right.m[1][0] + left[2] * right.m[2][0];
    r.v[1] = left[0] * right.m[0][1] + left[1] * right.m[1][1] + left[2] * right.m[2][1];
    r.v[2] = left[0] * right.m[0][2] + left[1] * right.m[1][2] + left[2] * right.m[2][2];
    return r;
}

constexpr Vector4f operator*(Vector4f left, Matrix4f right) noexcept {
    Vector4f r;
    // 将向量视为行向量，左乘矩阵
    r.v[0] = left[0] * right.m[0][0] + left[1] * right.m[1][0] + left[2] * right.m[2][0] + left[3] * right.m[3][0];
    r.v[1] = left[0] * right.m[0][1] + left[1] * right.m[1][1] + left[2] * right.m[2][1] + left[3] * right.m[3][1];
    r.v[2] = left[0] * right.m[0][2] + left[1] * right.m[1][2] + left[2] * right.m[2][2] + left[3] * right.m[3][2];
    r.v[3] = left[0] * right.m[0][3] + left[1] * right.m[1][3] + left[2] * right.m[2][3] + left[3] * right.m[3][3];
    return r;
}

} // namespace Goonya

template <>
struct std::formatter<Goonya::Matrix3f> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Matrix3f &m, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {}, {}, {}, {}, {}, {})", m.m[0][0], m.m[0][1], m.m[0][2],
                              m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2]);
    }
};

template <>
struct std::formatter<Goonya::Matrix4f> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const Goonya::Matrix4f &m, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})", m.m[0][0],
                              m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][0],
                              m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
    }
};