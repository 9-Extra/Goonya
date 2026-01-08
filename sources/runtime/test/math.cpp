#include <cmath>
#include <gtest/gtest.h>

#include "PrintTo.h"
#include "core/cgmath/cgmath.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/transform.h"
#include "core/cgmath/vector.h"

using namespace Goonya;

TEST(Vector, normalize) { EXPECT_FLOAT_EQ(Vector3f(1, 2, 3).normalize().length(), 1.0f); }

TEST(Quaternion, from_rotation) {
    Quaternion q1 = Quaternion::from_rotation(Vector3f{1, 2, 3}.normalize(), 0);
    Quaternion q2 = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
    EXPECT_EQ(q1, q2);

    q1 = Quaternion::from_rotation({0, 0, 1}, to_radian(90));
    q2 = {0, 0, std::sqrtf(2) / 2, std::sqrtf(2) / 2};
    EXPECT_EQ(q1, q2);
}

TEST(Quaternion, from_eular) {
    Quaternion q1 = Quaternion::from_eular(Vector3f(to_radian(90), to_radian(90), 0));
    Quaternion q2 = Quaternion{0.5f, 0.5f, 0.5f, 0.5f};
    EXPECT_EQ(q1, q2);

    q1 = Quaternion::from_eular(Vector3f(to_radian(108), to_radian(23), to_radian(80)));
    q2 = Quaternion{0.68262684, -0.4198172, 0.49379304, 0.33755374};
    EXPECT_EQ(q1, q2);

    Quaternion qx = Quaternion::from_eular(Vector3f(to_radian(108), 0, 0));
    Quaternion qy = Quaternion::from_eular(Vector3f(0, to_radian(23), 0));
    Quaternion qz = Quaternion::from_eular(Vector3f(0, 0, to_radian(80)));
    // 四元数相乘相当于内旋，与外旋方向相反，为ZYX
    EXPECT_EQ(qx * qy * qz, q2);
}

TEST(Quaternion, apply) {
    Vector3f point{0, 0, -1};
    Quaternion q1 = Quaternion::from_rotation(Vector3f(0, 1, 0), to_radian(90));
    EXPECT_EQ(point.apply(q1), Vector3f(-1, 0, 0));

    Quaternion qx = Quaternion::from_eular(Vector3f(to_radian(108), 0, 0));
    Quaternion qy = Quaternion::from_eular(Vector3f(0, to_radian(23), 0));
    Quaternion qz = Quaternion::from_eular(Vector3f(0, 0, to_radian(80)));
    // 在级联的情况下，apply与乘法反向
    EXPECT_EQ(point.apply(qz).apply(qy).apply(qx), qx * qy * qz * point);
}

TEST(Quaternion, to_matrix_and_resolve) {
    Vector3f point{0, 0, -1};
    Quaternion q1 = Quaternion::from_eular(Vector3f(to_radian(23), to_radian(45), to_radian(90)));
    Quaternion q2 = Quaternion::from_eular(Vector3f(to_radian(108), to_radian(23), to_radian(80)));
    EXPECT_EQ(point * Matrix3f::from_quaternion(q1), point.apply(q1));
    EXPECT_EQ(Matrix3f::from_quaternion(q1) * Matrix3f::from_quaternion(q2), Matrix3f::from_quaternion(q1.apply(q2)));

    EXPECT_EQ(Matrix3f::from_quaternion(q1).resolve_rotation_normalized(), q1);
    EXPECT_EQ(Matrix4f{Matrix3f::from_quaternion(q1)}.resolve_rotation_normalized(), q1);
    EXPECT_EQ(Matrix4f{Matrix3f::from_quaternion(q1)}.transpose().resolve_rotation_normalized(), q1.conjugate());

    EXPECT_EQ((Matrix4f::identity().translate({123, 1, 0}).rotate(q1).translate({115, 514, 221}))
                  .resolve_rotation_normalized(),
              q1);

    // resolve_rotation_normalized不支持没有归一化的矩阵
    Matrix4f unnormalized_matrix = Matrix4f::identity().scale({123, 1, 1}).rotate(q1);
    EXPECT_NE(unnormalized_matrix.resolve_rotation_normalized(), q1);
    // 用Transform分解
    EXPECT_EQ(Transform::from_matrix(unnormalized_matrix).rotation, q1);

    // 因为误差导致接近0的情况
    Matrix4f mat{-0.999999523,
                 -0.000418676762,
                 0.000213881052,
                 0,
                 -0.000214216896,
                 0.000802397727,
                 -0.999999523,
                 0,
                 0.00041850502,
                 -0.999999403,
                 -0.000802159309,
                 0,
                 0,
                 5,
                 -10,
                 1};
    Quaternion quat{-0.0, 0.707390308, -0.706822991, 0.00029901};
    EXPECT_EQ(Transform::from_matrix(mat).rotation, quat);
}

TEST(Matrix, determinant_and_inverse) {
    Matrix3f mat3{5, 6, 7, 10, 9, 8, 3, 3, 3};
    EXPECT_FLOAT_EQ(mat3.determinant(), 0);
    EXPECT_FALSE(mat3.inverse());

    mat3.m[2][2] = 5;
    EXPECT_FLOAT_EQ(mat3.determinant(), -30.0f);
    EXPECT_EQ(mat3.inverse().value() * mat3, Matrix3f::identity());

    Matrix4f mat4{1, 2, 3, 4, 5, 1, 7, 8, 9, 10, 1, 12, 13, 14, 15, 1};
    EXPECT_FLOAT_EQ(mat4.determinant(), -4350.f);
    EXPECT_EQ(mat4 * mat4.inverse().value(), Matrix4f::identity());
}

TEST(Matrix, translate_scale) {
    const Vector3f t1{95.7264f, 40.8074f, 0.1644f};
    const Vector3f t2{-1.f, -0.41863245f, -0.3396743f};
    const float scale = 2.f / 191.1696014404297f;
    Goonya::Matrix4f transform = Goonya::Matrix4f::identity().translate(t1).scale(scale).translate(t2);
    const Vector3f ori{123, 456, 1000};
    EXPECT_EQ((Vector4f{ori, 1} * transform).perspective_division(), ((ori + t1) * scale + t2));
}

TEST(Transform, to_and_from_matrix) {
    Vector3f pos{114, 514, 221};
    Quaternion rot = Quaternion::from_eular(Vector3f(to_radian(23), to_radian(45), to_radian(90)));
    Vector3f scale = {3, 4, 5};

    Transform transform{pos, rot, scale};
    Transform resolved = Transform::from_matrix(transform.model_matrix());

    EXPECT_EQ(resolved.position, transform.position);
    EXPECT_EQ(resolved.rotation, transform.rotation);
    EXPECT_EQ(resolved.scale, transform.scale);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}