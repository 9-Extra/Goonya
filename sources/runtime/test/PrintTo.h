// IWYU pragma: always_keep
#pragma once
#include "core/cgmath/cgmath.h"

namespace Goonya {

// 为gtest编译格式化函数，它会自动找到这个
void PrintTo(const Quaternion &quad, std::ostream *os) { *os << std::format("{}", quad); }
void PrintTo(const Vector3f &vec, std::ostream *os) { *os << std::format("{}", vec); }
void PrintTo(const Matrix3f &mat, std::ostream *os) { *os << std::format("{}", mat); }
} // namespace Goonya