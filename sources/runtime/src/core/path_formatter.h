// IWYU pragma: always_keep
#pragma once
#include "as_u8string.h"

#include <filesystem>
#include <format>

// 假设不是c++26

template <>
struct std::formatter<std::filesystem::path> {
    constexpr auto parse(std::format_parse_context &context) /*NOLINT*/ { return context.begin(); }
    template <typename FormatContext>
    auto format(const std::filesystem::path &path, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", Goonya::as_string_view(path.generic_u8string()));
    }
};