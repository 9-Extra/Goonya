#pragma once

#include <string>
#include <string_view>

namespace Goonya {

// 编译器选项保证内部编码使用utf-8，所以char == char8_t，可以相互转化
inline std::u8string_view as_u8string_view(const std::string &str) noexcept {
    return {(char8_t *)str.data(), str.size()};
}

inline std::u8string_view as_u8string_view(std::string_view str) noexcept {
    return {(char8_t *)str.data(), str.size()};
}

inline std::string_view as_string_view(const std::u8string &str) noexcept { return {(char *)str.data(), str.size()}; }

inline std::string_view as_string_view(std::u8string_view str) noexcept { return {(char *)str.data(), str.size()}; }

} // namespace Goonya