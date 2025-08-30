#pragma once

#include "core/cgmath.h"
#include <utility>

namespace Craft {

enum class Direction { DOWN = 0, UP = 1, NORTH = 2, SOUTH = 3, WEST = 4, EAST = 5 };

inline Goonya::Vector3f get_direction_vector(Direction d) noexcept {
    switch (d) {
    case Direction::DOWN:
        return {0, -1, 0};
    case Direction::UP:
        return {0, 1, 0};
    case Direction::NORTH:
        return {0, 0, -1}; // 我们认为-1为南方，即摄像机初始方向是面向南方的
    case Direction::SOUTH:
        return {0, 0, 1};
    case Direction::WEST:
        return {-1, 0, 0};
    case Direction::EAST:
        return {1, 0, 0};
    }
    std::unreachable();
}

} // namespace Craft

template <>
struct std::formatter<Craft::Direction> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::Direction &block, FormatContext &ctx) const {
        switch (block) {
        case Craft::Direction::DOWN:{
            return std::format_to(ctx.out(), "down");
        }
        case Craft::Direction::UP:{
            return std::format_to(ctx.out(), "up");
        }
        case Craft::Direction::NORTH:{
            return std::format_to(ctx.out(), "north");
        }
        case Craft::Direction::SOUTH:{
            return std::format_to(ctx.out(), "south");
        }
        case Craft::Direction::WEST:{
            return std::format_to(ctx.out(), "west");
        }
        case Craft::Direction::EAST:{
            return std::format_to(ctx.out(), "east");
        }
        }
        std::unreachable();
    }
};