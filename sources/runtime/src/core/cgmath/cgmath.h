#pragma once

#include "matrix.h"     // IWYU pragma: export
#include "quaternion.h" // IWYU pragma: export
#include "vector.h"     // IWYU pragma: export

constexpr float to_radian(float angle) { return angle / 180.0f * 3.1415926535f; }