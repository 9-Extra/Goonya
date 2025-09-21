#pragma once

#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "core/RefCount.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace Craft {

struct BlockInnerPos {
private:
    static constexpr uint32_t CHUNK_OFFSET_MASH = CHUNK_WIDTH - 1;
    // ------------------
    uint32_t index;

public:
    constexpr BlockInnerPos(uint32_t index) noexcept // NOLINT
        : index(index) {}

    constexpr BlockPos as_offset() const noexcept {
        [[assume((index >> CHUNK_WIDTH_OFFSET * 2 & CHUNK_OFFSET_MASH) == (index >> CHUNK_WIDTH_OFFSET * 2))]];
        return {int32_t((index >> CHUNK_WIDTH_OFFSET * 2) & CHUNK_OFFSET_MASH),
                int32_t((index >> CHUNK_WIDTH_OFFSET) & CHUNK_OFFSET_MASH), int32_t(index & CHUNK_OFFSET_MASH)};
    }

    constexpr explicit BlockInnerPos(BlockPos pos) noexcept
        : index(((pos.x & CHUNK_OFFSET_MASH) << CHUNK_WIDTH_OFFSET * 2) |
                ((pos.y & CHUNK_OFFSET_MASH) << CHUNK_WIDTH_OFFSET) | (pos.z & CHUNK_OFFSET_MASH)) {}

    constexpr uint32_t as_index() const noexcept { return index; }

    constexpr bool operator==(const BlockInnerPos &pos) const noexcept = default;

    struct Hasher {
        size_t operator()(BlockInnerPos pos) const noexcept { return (size_t)pos.index; }
    };

private:
};

enum class ChunkState { EMPTY, GENERATED };
enum class SetBlockOption {
    DIRECT,
};

struct Chunk : public RefCount {
public:
    ChunkState current_state = ChunkState::EMPTY;
    const ChunkPos chunk_pos;

    std::array<BlockState *, CHUNK_BLOCK_COUNT> block_states;
    std::unordered_map<BlockInnerPos, int, BlockInnerPos::Hasher> block_entities;

public:
    explicit Chunk(ChunkPos chunk_pos) : chunk_pos(chunk_pos), block_states() {};
    Chunk(Chunk &) = delete;

    BlockState *get_block_state(BlockInnerPos pos) const noexcept { return block_states[pos.as_index()]; }
    BlockState *get_block_state(BlockPos pos)  const noexcept { return get_block_state(BlockInnerPos(pos)); }
    BlockState *set_block_state(BlockInnerPos pos, BlockState *state, SetBlockOption option) noexcept {
        BlockState *old = block_states[pos.as_index()];
        block_states[pos.as_index()] = state;
        return old;
    }
    BlockState *set_block_state(BlockPos pos, BlockState *state, SetBlockOption option) noexcept {
        return set_block_state(BlockInnerPos(pos), state, option);
    }

private:
};

} // namespace Craft