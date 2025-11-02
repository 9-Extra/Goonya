#include <cmath>
#include <format>
#include <gtest/gtest.h>

#include "core/cgmath.h"

using namespace Goonya;

namespace Goonya {

// 为gtest编译格式化函数，它会自动找到这个
void PrintTo(const Quaternion &quad, std::ostream *os) { *os << std::format("{}", quad); }
void PrintTo(const Vector3f &vec, std::ostream *os) { *os << std::format("{}", vec); }
void PrintTo(const Matrix3 &mat, std::ostream *os) { *os << std::format("{}", mat); }
} // namespace Goonya

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
    EXPECT_EQ(qx * qy * qz, q2);

    Vector3f point{0, 0, -1};
    EXPECT_EQ(point.apply(qz).apply(qy).apply(qx), point.apply(q2));
}

TEST(Quaternion, apply) {
    Vector3f point{0, 0, -1};
    Quaternion q1 = Quaternion::from_rotation(Vector3f(0, 1, 0), to_radian(90));
    EXPECT_EQ(point.apply(q1), Vector3f(-1, 0, 0)); 
}

TEST(Quaternion, serial) {
    Vector3f point{0, 0, -1};
    Quaternion q1 = Quaternion::from_rotation(Vector3f(0, 1, 0), to_radian(90));
    Quaternion q2 = Quaternion::from_rotation(Vector3f(0, 0, 1), to_radian(-90));
    EXPECT_EQ(point.apply(q2 * q1), point.apply(q1).apply(q2));
}

TEST(Quaternion, to_matrix) {
    Vector3f point{0, 0, -1};
    Quaternion q1 = Quaternion::from_eular(Vector3f(to_radian(23), to_radian(45), to_radian(90)));
    Quaternion q2 = Quaternion::from_eular(Vector3f(to_radian(108), to_radian(23), to_radian(80)));
    EXPECT_EQ(point * Matrix3::rotate(q1), point.apply(q1));
    EXPECT_EQ(Matrix3::rotate(q1) * Matrix3::rotate(q2), Matrix3::rotate(q2 * q1));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}