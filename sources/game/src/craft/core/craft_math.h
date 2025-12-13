#pragma once

#include <cstdint>

namespace Craft {

constexpr int64_t get_seed(int32_t x, int32_t y, int32_t z) {
    int64_t i = ((int64_t)x * 3129871) ^ (int64_t)z * 116129781 ^ (int64_t)y;
    i = i * i * 42317861 + i * 11;
    return i >> 16;
}

// splitmix64算法，使随机数更加均匀
constexpr uint64_t splitmix64(uint64_t x) {
    uint64_t z = x + 0x9e3779b97f4a7c15; // 黄金分割常数
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

} // namespace Craft