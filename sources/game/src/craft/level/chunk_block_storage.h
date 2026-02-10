#pragma once

#include "craft/block/blockstate.h"
#include "craft/core/core.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Craft {

struct ChunkBlockStorage final {
    enum class Policy { ALL_AIR, ONE_BIT, TWO_BIT, FOUR_BIT, EIGHT_BIT, SIXTEEN_BIT, DIRECT };
    Policy policy = Policy::ALL_AIR;
    std::vector<BlockState *> block_states;
    std::unordered_map<BlockState *, uint32_t> block_state_index_map;
    uint16_t *raw_data = nullptr;
    static_assert(CHUNK_BLOCK_COUNT % 16 == 0);

    ChunkBlockStorage() = default;
    ChunkBlockStorage(ChunkBlockStorage &other);
    ~ChunkBlockStorage() { delete[] raw_data; }

    BlockState *get_block_state(uint32_t pos) const noexcept;

    void set_block_state(uint32_t pos, BlockState *state) noexcept;

private:
    constexpr static uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();
    uint32_t get_block_index(BlockState *state) const noexcept;
    void set_block_index(uint32_t pos, uint32_t state_index) noexcept;
};

} // namespace Craft