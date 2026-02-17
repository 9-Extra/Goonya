#include <cmath>
#include <gtest/gtest.h>

#include "PrintTo.h"
#include "core/cgmath/cgmath.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/transform.h"
#include "core/cgmath/vector.h"

// NOLINTBEGIN(cert-flp30-c)

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

    // resolve_rotation_normalize不支持没有归一化的矩阵
    Matrix4f unnormalize_matrix = Matrix4f::identity().scale({123, 1, 1}).rotate(q1);
    EXPECT_NE(unnormalize_matrix.resolve_rotation_normalized(), q1);
    // 用Transform分解
    EXPECT_EQ(Transform::from_matrix(unnormalize_matrix).rotation, q1);

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

class QuaternionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置精度阈值
        epsilon = 1e-5f;
    }

    float epsilon;

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    float angle_between(const Quaternion &q1, const Quaternion &q2) {
        Quaternion q1n = q1.normalize();
        Quaternion q2n = q2.normalize();
        return Quaternion::angle_between(q1n, q2n);
    }
};

// 测试1：边界条件测试 - t=0和t=1应该返回原始四元数
TEST_F(QuaternionTest, BoundaryConditions) {
    Quaternion q1(0.0, 0.0, 0.0, 1.0);
    Quaternion q2(1.0, 0.0, 0.0, 0.0);

    // t=0 应该返回q1
    Quaternion result0 = Quaternion::slerp(q1, q2, 0.0);
    EXPECT_TRUE(Quaternion::rotation_equal(result0, q1));

    // t=1 应该返回q2
    Quaternion result1 = Quaternion::slerp(q1, q2, 1.0);
    EXPECT_TRUE(Quaternion::rotation_equal(result1, q2));
}

// 测试2：恒等四元数插值
TEST_F(QuaternionTest, IdentityInterpolation) {
    Quaternion q1(0.0, 0.0, 0.0, 1.0); // 恒等旋转
    Quaternion q2(0.0, 1.0, 0.0, 0.0); // 绕y轴旋转180度

    float t = 0.5;
    Quaternion result = Quaternion::slerp(q1, q2, t);

    // 验证结果为单位四元数
    float norm = result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z;
    EXPECT_NEAR(norm, 1.0, epsilon);
}

// 测试3：线性插值退化情况（非常接近的四元数）
TEST_F(QuaternionTest, NearlyIdenticalQuaternions) {
    Quaternion q1(0.01, 0.0, 0.0, 0.9999);
    Quaternion q2(0.02, 0.0, 0.0, 0.9998);

    float t = 0.3;
    Quaternion result = Quaternion::slerp(q1, q2, t);

    // 验证结果接近线性插值
    Quaternion expected = (q1 * (1.0 - t) + q2 * t).normalize();

    EXPECT_NEAR(result.dot(expected), 1.0, epsilon);
}

// 测试4：负点积情况（最短路径测试）
TEST_F(QuaternionTest, ShortestPath) {
    // 创建两个代表相同物理旋转但符号相反的四元数
    Quaternion q1(0.7071, 0.0, 0.0, 0.7071);   // 绕x轴90度
    Quaternion q2(-0.7071, 0.0, 0.0, -0.7071); // 相同的旋转，但符号相反

    float t = 0.5;
    Quaternion result = Quaternion::slerp(q1, q2, t);

    // 验证插值结果应该接近q1（因为q2被取反后与q1相同）
    float dot = q1.normalize().dot(result.normalize());
    EXPECT_NEAR(dot, 1.0, epsilon);
}

// 测试5：对径点测试（180度旋转）
TEST_F(QuaternionTest, AntipodalPoints) {
    Quaternion q1(0.0, 0.0, 0.0, 1.0); // 0度
    Quaternion q2(1.0, 0.0, 0.0, 0.0); // 绕x轴180度

    // 测试多个插值点
    std::vector<float> t_values = {0.25, 0.5, 0.75};

    for (float t : t_values) {
        Quaternion result = Quaternion::slerp(q1, q2, t);

        // 验证插值结果确实是单位四元数
        float norm = result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z;
        EXPECT_NEAR(norm, 1.0, epsilon);

        // 验证插值角度线性变化
        float angle = angle_between(q1, result);
        float expected_angle = t * M_PI; // 180度 = π弧度
        EXPECT_NEAR(angle, expected_angle, epsilon);
    }
}

// 测试6：连续性测试
TEST_F(QuaternionTest, Continuity) {
    Quaternion q1(0.7071, 0.0, 0.0, 0.7071);
    Quaternion q2(0.0, 0.7071, 0.7071, 0.0);

    q1 = q1.normalize();
    q2 = q2.normalize();

    float t_prev = 0.0f;
    Quaternion result_prev = Quaternion::slerp(q1, q2, t_prev);

    // 测试小的t增量，验证平滑变化
    for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
        Quaternion result = Quaternion::slerp(q1, q2, t);

        // 连续点之间的角度差应该很小
        float angle_diff = angle_between(result_prev, result);
        EXPECT_LT(angle_diff, to_radian(18.1f)); // 应该小于18度

        result_prev = result;
    }
}

// 测试7：对称性测试 - SLERP应该是对称的
TEST_F(QuaternionTest, Symmetry) {
    Quaternion q1(0.7071, 0.0, 0.0, 0.7071);
    Quaternion q2(0.7071, 0.7071, 0.0, 0.0);

    float t = 0.3;
    Quaternion result1 = Quaternion::slerp(q1, q2, t);
    Quaternion result2 = Quaternion::slerp(q2, q1, 1.0f - t);

    // 结果应该相同（允许双重覆盖）
    EXPECT_TRUE(Quaternion::rotation_equal(result1, result2));
}

// 测试8：大角度差测试
TEST_F(QuaternionTest, LargeAngleDifference) {
    Quaternion q1(0.0, 0.0, 0.0, 1.0); // 0度
    Quaternion q2(0.0, 0.0, 1.0, 0.0); // 绕z轴180度

    float t = 0.5f;
    Quaternion result = Quaternion::slerp(q1, q2, t);

    // 期望结果应该是绕z轴90度
    float expected_w = std::cosf(M_PI / 4); // 45度
    float expected_z = std::sinf(M_PI / 4); // 45度

    EXPECT_NEAR(std::abs(result.w), expected_w, epsilon);
    EXPECT_NEAR(std::abs(result.z), expected_z, epsilon);
}

// 测试9：数值稳定性测试 - 接近但不在边界
TEST_F(QuaternionTest, NumericalStability) {
    // 测试接近1但不到1的cosOmega值
    Quaternion q1(0.001f, 0.0f, 0.0f, 0.999999f);
    Quaternion q2(0.002f, 0.0f, 0.0f, 0.999998f);

    for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
        EXPECT_NO_THROW({
            [[maybe_unused]] Quaternion result = Quaternion::slerp(q1, q2, t);
            SUCCEED();
        });
    }
}

// 测试10：恒等变换的一致性
TEST_F(QuaternionTest, ConsistencyWithIdentity) {
    Quaternion identity(0.0f, 0.0f, 0.0f, 1.0f);
    Quaternion q(0.7071f, 0.0f, 0.0f, 0.7071f);

    // 从恒等到q的插值
    for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
        Quaternion result = Quaternion::slerp(identity, q, t);

        // 预期的角度应该是线性变化
        float expected_angle = t * M_PI / 2; // 90度 = π/2
        float actual_angle = 2.0f * std::acosf(result.w);

        EXPECT_NEAR(actual_angle, expected_angle, epsilon);
    }
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

// NOLINTEND(cert-flp30-c)

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}