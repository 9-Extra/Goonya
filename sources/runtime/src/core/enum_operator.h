#pragma once
#include <utility> // IWYU pragma: keep 宏里的std::to_underlying

// 不能用在类定义的内部
#define DECLARE_ENUM_OPERATORS(ENUM_NAME)                                                                              \
    constexpr ENUM_NAME operator|(const ENUM_NAME lhs, const ENUM_NAME rhs) {                                          \
        return ENUM_NAME{std::to_underlying(rhs) | std::to_underlying(lhs)};                                           \
    }                                                                                                                  \
                                                                                                                       \
    constexpr ENUM_NAME operator&(const ENUM_NAME lhs, const ENUM_NAME rhs) {                                          \
        return ENUM_NAME{std::to_underlying(rhs) & std::to_underlying(lhs)};                                           \
    }                                                                                                                  \
                                                                                                                       \
    constexpr ENUM_NAME operator^(const ENUM_NAME lhs, const ENUM_NAME rhs) {                                          \
        return ENUM_NAME{std::to_underlying(rhs) ^ std::to_underlying(lhs)};                                           \
    }                                                                                                                  \
                                                                                                                       \
    constexpr ENUM_NAME operator~(const ENUM_NAME lhs) { return ENUM_NAME{~std::to_underlying(lhs)}; }                 \
    constexpr ENUM_NAME &operator|=(ENUM_NAME &lhs, const ENUM_NAME rhs) {                                             \
        lhs = lhs | rhs;                                                                                               \
        return lhs;                                                                                                    \
    }                                                                                                                  \
    constexpr ENUM_NAME &operator&=(ENUM_NAME &lhs, const ENUM_NAME rhs) {                                             \
        lhs = lhs & rhs;                                                                                               \
        return lhs;                                                                                                    \
    }                                                                                                                  \
    constexpr ENUM_NAME &operator^=(ENUM_NAME &lhs, const ENUM_NAME rhs) {                                             \
        lhs = lhs ^ rhs;                                                                                               \
        return lhs;                                                                                                    \
    }                                                                                                                  \
    constexpr bool contain(const ENUM_NAME lhs, const ENUM_NAME rhs) {                                                 \
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) == std::to_underlying(rhs);                         \
    }
// 抑制续行符出现在文件末尾的警告