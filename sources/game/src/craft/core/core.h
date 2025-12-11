#pragma once

#include "core/cgmath/cgmath.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <generator>
#include <utility>
#include <array>

namespace Craft {

enum class Direction { DOWN = 0, UP = 1, NORTH = 2, SOUTH = 3, WEST = 4, EAST = 5 };
constexpr std::array<Direction, 6> DIRECTION_VALUES{Direction::DOWN,  Direction::UP,   Direction::NORTH,
                                                    Direction::SOUTH, Direction::WEST, Direction::EAST};

inline constexpr Direction direction_opposite(Direction d) noexcept {
    switch (d) {
    case Direction::DOWN:
        return Direction::UP;
    case Direction::UP:
        return Direction::DOWN;
    case Direction::NORTH:
        return Direction::SOUTH;
    case Direction::SOUTH:
        return Direction::NORTH;
    case Direction::WEST:
        return Direction::EAST;
    case Direction::EAST:
        return Direction::WEST;
    }
    std::unreachable();
}

struct Vector3i {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    Vector3i() noexcept = default;
    constexpr Vector3i(int32_t x, int32_t y, int32_t z) noexcept : x(x), y(y), z(z) {}
    constexpr explicit Vector3i(Goonya::Vector3f vec_pos) noexcept
        : x((int32_t)vec_pos.x), y((int32_t)vec_pos.y), z((int32_t)vec_pos.z) {}

    constexpr bool operator==(const Vector3i &pos) const noexcept = default;

    constexpr Vector3i operator+(int32_t offset) const noexcept { return {x + offset, y + offset, z + offset}; }
    constexpr Vector3i operator-(int32_t offset) const noexcept { return {x - offset, y - offset, z - offset}; }
    constexpr Vector3i operator+(Vector3i offset) const noexcept { return {x + offset.x, y + offset.y, z + offset.z}; }
    constexpr Vector3i operator-(Vector3i offset) const noexcept { return {x - offset.x, y - offset.y, z - offset.z}; }

    constexpr operator Goonya::Vector3f() const noexcept /*NOLINT: implict*/ { return {(float)x, (float)y, (float)z}; }

    int32_t distance_manhattan(Vector3i vec) const noexcept {
        return std::abs(x - vec.x) + std::abs(y - vec.y) + std::abs(z - vec.z);
    }

    bool is_in_region(Vector3i center, int32_t distance) const noexcept {
        Vector3i offset = *this - center;
        return std::abs(offset.x) <= distance && std::abs(offset.y) <= distance && std::abs(offset.z) <= distance;
    }

    constexpr Vector3i move(Direction direction, int32_t step = 1) const noexcept {
        switch (direction) {
        case Direction::DOWN:
            return *this + Vector3i{0, -step, 0};
        case Direction::UP:
            return *this + Vector3i{0, step, 0};
        case Direction::NORTH:
            return *this + Vector3i{0, 0, -step};
        case Direction::SOUTH:
            return *this + Vector3i{0, 0, step};
        case Direction::WEST:
            return *this + Vector3i{-step, 0, 0};
        case Direction::EAST:
            return *this + Vector3i{step, 0, 0};
        }
        std::unreachable();
    }

    template <std::derived_from<Vector3i> T>
    static std::generator<T> iterate_region(T start, T end) noexcept {
        for (int32_t x = start.x; x < end.x; x++) {
            for (int32_t z = start.z; z < end.z; z++) {
                for (int32_t y = start.y; y < end.y; y++) {
                    co_yield {x, y, z};
                }
            }
        }
    }
};

constexpr Vector3i get_direction_vector(Direction d) noexcept {
    switch (d) {
    case Direction::DOWN:
        return {0, -1, 0};
    case Direction::UP:
        return {0, 1, 0};
    case Direction::NORTH:
        return {0, 0, -1}; // 我们认为-1为北方，即摄像机初始方向是面向北方的，与MC保持一致
    case Direction::SOUTH:
        return {0, 0, 1};
    case Direction::WEST:
        return {-1, 0, 0};
    case Direction::EAST:
        return {1, 0, 0};
    }
    std::unreachable();
}

struct BlockPos : public Vector3i {
    using Vector3i::Vector3i;
    constexpr explicit BlockPos(Vector3i vec) noexcept : Vector3i(vec) {}

    constexpr Goonya::Vector3f get_center() const noexcept { return {x + 0.5f, y + 0.5f, z + 0.5f}; }
};

constexpr uint32_t CHUNK_WIDTH_OFFSET = 5;
constexpr uint32_t CHUNK_WIDTH = 1 << CHUNK_WIDTH_OFFSET; // 长宽高都是这个
constexpr uint32_t CHUNK_BLOCK_COUNT = CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH;

struct ChunkPos : public Vector3i {

    constexpr ChunkPos() noexcept = default;
    constexpr ChunkPos(int32_t x, int32_t y, int32_t z) noexcept : Vector3i(x, y, z) {}
    explicit constexpr ChunkPos(BlockPos pos) noexcept
        : Vector3i(pos.x >> CHUNK_WIDTH_OFFSET, pos.y >> CHUNK_WIDTH_OFFSET, pos.z >> CHUNK_WIDTH_OFFSET) {}

    constexpr ChunkPos operator+(int32_t offset) const noexcept { return {x + offset, y + offset, z + offset}; }
    constexpr ChunkPos operator-(int32_t offset) const noexcept { return {x - offset, y - offset, z - offset}; }
    constexpr ChunkPos operator+(Vector3i offset) const noexcept { return {x + offset.x, y + offset.y, z + offset.z}; }
    constexpr ChunkPos operator-(Vector3i offset) const noexcept { return {x - offset.x, y - offset.y, z - offset.z}; }

    constexpr BlockPos get_start_pos() const noexcept {
        return {x << CHUNK_WIDTH_OFFSET, y << CHUNK_WIDTH_OFFSET, z << CHUNK_WIDTH_OFFSET};
    }
    // end_pos不在本Chunk内部
    constexpr BlockPos get_end_pos() const noexcept {
        return BlockPos{get_start_pos() + BlockPos{CHUNK_WIDTH, CHUNK_WIDTH, CHUNK_WIDTH}};
    }
};

using GameTime = uint64_t;

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
    size_t operator()(Craft::BlockPos pos) const noexcept { return std::hash<Craft::Vector3i>{}(pos); }
};

template <>
struct std::hash<Craft::ChunkPos> {
    size_t operator()(Craft::ChunkPos pos) const noexcept { return std::hash<Craft::Vector3i>{}(pos); }
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
