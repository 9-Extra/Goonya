#pragma once

#include "core/cgmath.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <utility>

namespace Craft {

enum class Direction { DOWN = 0, UP = 1, NORTH = 2, SOUTH = 3, WEST = 4, EAST = 5 };

inline constexpr Goonya::Vector3f get_direction_vector(Direction d) noexcept {
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

struct Vector3i{
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    Vector3i() noexcept = default;
    Vector3i(int32_t x, int32_t y, int32_t z): x(x), y(y), z(z) {}
    explicit Vector3i(Goonya::Vector3f vec_pos): x((int32_t)vec_pos.x), y((int32_t)vec_pos.y), z((int32_t)vec_pos.z) {}

    constexpr bool operator==(const Vector3i &pos) const noexcept = default;
 
    constexpr Vector3i operator+(Vector3i offset) const noexcept { return {x + offset.x, y + offset.y, z + offset.z}; }

    int32_t distance_manhattan(Vector3i vec) const noexcept{
        return std::abs(x - vec.x) + std::abs(y - vec.y) + std::abs(z - vec.z);
    }

};

struct BlockPos: public Vector3i {
    using Vector3i::Vector3i;
    explicit BlockPos(Vector3i vec): Vector3i(vec) {}

    constexpr Goonya::Vector3f get_center() const noexcept { return {x + 0.5f, y + 0.5f, z + 0.5f}; }    
};

constexpr uint32_t CHUNK_WIDTH_OFFSET = 5;
constexpr uint32_t CHUNK_WIDTH = 1 << CHUNK_WIDTH_OFFSET; // 长宽高都是这个
constexpr uint32_t CHUNK_BLOCK_COUNT = CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH;

struct ChunkPos: public Vector3i {
    using Vector3i::Vector3i;
    explicit ChunkPos(Vector3i vec): Vector3i(vec) {}

    constexpr BlockPos get_start_pos() const noexcept{
        return {x << CHUNK_WIDTH_OFFSET, y << CHUNK_WIDTH_OFFSET, z << CHUNK_WIDTH_OFFSET};
    }

    constexpr explicit ChunkPos(BlockPos pos) noexcept : ChunkPos(pos.x >> CHUNK_WIDTH_OFFSET, pos.y >> CHUNK_WIDTH_OFFSET, pos.z >> CHUNK_WIDTH_OFFSET) {
        // 需要右移的语义为算术右移
        static_assert(-1 >> 1 == -1, ">> operator must on int32_t be arithmetic shift");
    }
};

} // namespace Craft

template <>
struct std::formatter<Craft::Direction> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::Direction &block, FormatContext &ctx) const {
        switch (block) {
        case Craft::Direction::DOWN: {
            return std::format_to(ctx.out(), "down");
        }
        case Craft::Direction::UP: {
            return std::format_to(ctx.out(), "up");
        }
        case Craft::Direction::NORTH: {
            return std::format_to(ctx.out(), "north");
        }
        case Craft::Direction::SOUTH: {
            return std::format_to(ctx.out(), "south");
        }
        case Craft::Direction::WEST: {
            return std::format_to(ctx.out(), "west");
        }
        case Craft::Direction::EAST: {
            return std::format_to(ctx.out(), "east");
        }
        }
        std::unreachable();
    }
};

template <>
struct std::hash<Craft::Vector3i> {
    size_t operator()(Craft::Vector3i pos) const noexcept {
        return (size_t)pos.x ^ (size_t)pos.y << 4 ^ (size_t)pos.z << 8;
    }
};

template <>
struct std::hash<Craft::BlockPos> {
    size_t operator()(Craft::BlockPos pos) const noexcept {
        return std::hash<Craft::Vector3i>{}(pos);
    }
};

template <>
struct std::hash<Craft::ChunkPos> {
    size_t operator()(Craft::ChunkPos pos) const noexcept {
        return std::hash<Craft::Vector3i>{}(pos);
    }
};

template <>
struct std::formatter<Craft::Vector3i> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::Vector3i &pos, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", pos.x, pos.y, pos.z);
    }
};

template <>
struct std::formatter<Craft::BlockPos> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::BlockPos &pos, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", pos.x, pos.y, pos.z);
    }
};

template <>
struct std::formatter<Craft::ChunkPos> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::ChunkPos &pos, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", pos.x, pos.y, pos.z);
    }
};


