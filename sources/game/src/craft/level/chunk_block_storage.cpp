#include "chunk_block_storage.h"

#include "craft/block/all_blocks.h"
#include "craft/block/blockstate.h"
#include <cstdint>

namespace Craft {

ChunkBlockStorage::ChunkBlockStorage(ChunkBlockStorage &other)
    : policy(other.policy), block_states(other.block_states), block_state_index_map(other.block_state_index_map) {
    raw_data = nullptr;
    if (other.raw_data) {
        switch (policy) {
        case Policy::ONE_BIT: {
            raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 16];
            for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 16; i++) {
                raw_data[i] = other.raw_data[i];
            }
            break;
        case Policy::TWO_BIT: {
            raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 8];
            for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 8; i++) {
                raw_data[i] = other.raw_data[i];
            }
            break;
        }
        case Policy::FOUR_BIT: {
            raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 4];
            for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 4; i++) {
                raw_data[i] = other.raw_data[i];
            }
            break;
        }
        case Policy::EIGHT_BIT: {
            raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 2];
            for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 2; i++) {
                raw_data[i] = other.raw_data[i];
            }
            break;
        }
        case Policy::SIXTEEN_BIT: {
            raw_data = new uint16_t[CHUNK_BLOCK_COUNT];
            for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT; i++) {
                raw_data[i] = other.raw_data[i];
            }
            break;
        }
        default:
            break;
        }
        }
    }
}
BlockState *ChunkBlockStorage::get_block_state(uint32_t pos) const noexcept {
    switch (policy) {
    case Policy::ALL_AIR:
        return Blocks::get().AIR->get_default_blockstate();
    case Policy::ONE_BIT:
        return block_states[uint8_t(raw_data[pos / 16] >> (pos % 16)) & 1];
    case Policy::TWO_BIT:
        return block_states[uint8_t(raw_data[pos / 8] >> (pos % 8 * 2)) & 3];
    case Policy::FOUR_BIT:
        return block_states[uint8_t(raw_data[pos / 4] >> (pos % 4 * 4)) & 15];
    case Policy::EIGHT_BIT:
        return block_states[uint8_t(raw_data[pos / 2] >> (pos % 2 * 8)) & 255];
    case Policy::SIXTEEN_BIT:
        return block_states[raw_data[pos]];
    case Policy::DIRECT:
        return block_states[pos];
    }
    std::unreachable();
}
void ChunkBlockStorage::set_block_state(uint32_t pos, BlockState *state) noexcept {
    if (policy == Policy::DIRECT) [[unlikely]] {
        block_states[pos] = state;
        return;
    }

    const uint32_t state_index = get_block_index(state);
    if (state_index != INVALID_INDEX) [[likely]] {
        set_block_index(pos, state_index);
    } else {
        switch (policy) {
        case Policy::ALL_AIR: {
            BlockState *air = Blocks::get().AIR->get_default_blockstate();
            if (state != air) {
                policy = Policy::ONE_BIT;
                raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 16];
                for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 16; i++) {
                    raw_data[i] = 0;
                }
                block_states.emplace_back(air); // 0号为空气
            } else {
                return; // 所有方块都是空气，不需要存储
            }
            break;
        }
        case Policy::ONE_BIT: {
            if (block_states.size() == 2) {
                policy = Policy::TWO_BIT;
                uint16_t *new_raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 8];
                for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 8; i++) {
                    new_raw_data[i] = 0;
                    uint8_t source = uint8_t(raw_data[i / 2] >> (i % 2 * 8));
                    for (uint32_t j = 0; j < 8; j++) {
                        new_raw_data[i] |= ((source >> j) & 1) << (j * 2);
                    }
                }
                delete[] raw_data;
                raw_data = new_raw_data;
            }
            break;
        }
        case Policy::TWO_BIT: {
            if (block_states.size() == 4) {
                policy = Policy::FOUR_BIT;
                uint16_t *new_raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 4];
                for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 4; i++) {
                    new_raw_data[i] = 0;
                    uint8_t source = raw_data[i / 2] >> (i % 2 * 8);
                    for (uint32_t j = 0; j < 4; j++) {
                        new_raw_data[i] |= ((source >> (j * 2)) & 3) << (j * 4);
                    }
                }
                delete[] raw_data;
                raw_data = new_raw_data;
            }
            break;
        }
        case Policy::FOUR_BIT: {
            if (block_states.size() == 16) {
                policy = Policy::EIGHT_BIT;
                uint16_t *new_raw_data = new uint16_t[CHUNK_BLOCK_COUNT / 2];
                for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT / 2; i++) {
                    new_raw_data[i] = 0;
                    uint8_t source = raw_data[i / 2] >> (i % 2 * 8);
                    for (uint32_t j = 0; j < 2; j++) {
                        new_raw_data[i] |= ((source >> (j * 4)) & 15) << (j * 8);
                    }
                }
                delete[] raw_data;
                raw_data = new_raw_data;

                for (auto [i, state_index] : std::views::enumerate(block_states)) {
                    block_state_index_map.emplace(state_index, i);
                }
            }
            break;
        }
        case Policy::EIGHT_BIT: {
            if (block_states.size() == 256) {
                policy = Policy::SIXTEEN_BIT;
                uint16_t *new_raw_data = new uint16_t[CHUNK_BLOCK_COUNT];
                for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT; i++) {
                    new_raw_data[i] = raw_data[i / 2] >> (i % 2 * 8);
                }
                delete[] raw_data;
                raw_data = new_raw_data;
            }
            break;
        }
        case Policy::SIXTEEN_BIT: {
            if (block_states.size() == 65536) {
                policy = Policy::DIRECT;
                std::vector<BlockState *> new_block_states(CHUNK_BLOCK_COUNT);
                for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT; i++) {
                    new_block_states[i] = block_states[raw_data[i]];
                }
                block_states = std::move(new_block_states);
                block_state_index_map.clear();
                delete[] raw_data;

                block_states[pos] = state;
                return;
            }
        }
        default:
            std::unreachable();
        }

        uint32_t state_index = (uint32_t)block_states.size();
        block_states.emplace_back(state);
        if (policy == Policy::EIGHT_BIT || policy == Policy::SIXTEEN_BIT) {
            block_state_index_map.emplace(state, state_index);
        }
        set_block_index(pos, state_index);
    }
}
uint32_t ChunkBlockStorage::get_block_index(BlockState *state) const noexcept {
    if (policy == Policy::EIGHT_BIT || policy == Policy::SIXTEEN_BIT) {
        auto iter = block_state_index_map.find(state);
        return iter != block_state_index_map.end() ? iter->second : INVALID_INDEX;
    }
    for (uint32_t i = 0; i < block_states.size(); i++) {
        if (block_states[i] == state) {
            return i;
        }
    }
    return INVALID_INDEX;
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void ChunkBlockStorage::set_block_index(uint32_t pos, uint32_t state_index) noexcept {
    switch (policy) {
    case Policy::ONE_BIT:
        raw_data[pos / 16] = (raw_data[pos / 16] & ~(uint16_t(1) << (pos % 16 * 1))) | (state_index << (pos % 16 * 1));
        break;
    case Policy::TWO_BIT:
        raw_data[pos / 8] = (raw_data[pos / 8] & ~(uint16_t(3) << (pos % 8 * 2))) | (state_index << (pos % 8 * 2));
        break;
    case Policy::FOUR_BIT:
        raw_data[pos / 4] = (raw_data[pos / 4] & ~(uint16_t(15) << (pos % 4 * 4))) | (state_index << (pos % 4 * 4));
        break;
    case Policy::EIGHT_BIT:
        raw_data[pos / 2] = (raw_data[pos / 2] & ~(uint16_t(255) << (pos % 2 * 8))) | (state_index << (pos % 2 * 8));
        break;
    case Policy::SIXTEEN_BIT:
        raw_data[pos] = state_index;
        break;
    default:
        std::unreachable();
    }
}

} // namespace Craft