#include <cmath>
#include <gtest/gtest.h>

#include "core/cgmath.h"
#include "PrintTo.h"

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
    EXPECT_EQ(point * Matrix3::rotate(q1), point.apply(q1));
    EXPECT_EQ(Matrix3::rotate(q1) * Matrix3::rotate(q2), Matrix3::rotate(q1.apply(q2)));

    EXPECT_EQ(Matrix3::rotate(q1).resolve_rotation(), q1);
    EXPECT_EQ(Matrix4::rotate(q1).resolve_rotation(), q1);
    EXPECT_EQ(Matrix4::rotate(q1).transpose().resolve_rotation(), q1.conjugate());
    
    EXPECT_EQ((Matrix4::translate({123, 1, 0}) * Matrix4::rotate(q1) * Matrix4::translate({115, 514, 221})).resolve_rotation(), q1);
    //todo: failed
    EXPECT_EQ((Matrix4::scale({123, 1, 1}) * Matrix4::rotate(q1) * Matrix4::scale({115, 514, 221})).resolve_rotation(), q1);
     
}

TEST(Matrix, determinant_and_inverse) {
    Matrix3 mat3{5, 6, 7, 10, 9, 8, 3, 3, 3};
    EXPECT_FLOAT_EQ(mat3.determinant(), 0);
    EXPECT_FALSE(mat3.inverse());

    mat3.m[2][2] = 5;
    EXPECT_FLOAT_EQ(mat3.determinant(), -30.0f);
    EXPECT_EQ(mat3.inverse().value() * mat3, Matrix3::identity());
    
    Matrix4 mat4{1, 2, 3, 4, 5, 1, 7, 8, 9, 10, 1, 12, 13, 14, 15, 1};
    EXPECT_FLOAT_EQ(mat4.determinant(), -4350.f);
    EXPECT_EQ(mat4 * mat4.inverse().value(), Matrix4::identity());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}