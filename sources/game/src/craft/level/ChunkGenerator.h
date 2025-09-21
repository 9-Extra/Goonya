#pragma once

#include "core/ThreadPool.h"
#include "chunk.h"
#include "craft/block/all_blocks.h"

#include <future>

namespace Craft {

class ChunkGenerator {
private:
    int32_t air_layer = 128;

public:
    ChunkGenerator() noexcept = default;
    ~ChunkGenerator() = default;

    std::future<Ref<Chunk>> process_chunk_async(const Ref<Chunk> &chunk) {
        return Goonya::THREAD_POOL.enqueue([this, chunk] { 
            return do_process_chunk(chunk); 
        });
    }

private:
    Ref<Chunk> do_process_chunk(Ref<Chunk> chunk) const {
        assert(chunk->current_state == ChunkState::EMPTY);

        // LOG_TRACE("正在生成{}处的区块", chunk->chunk_pos);

        for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT; i++) {
            BlockInnerPos inner_pos{i};
            BlockPos world_pos = BlockPos(chunk->chunk_pos.get_start_pos() + inner_pos.as_offset());
            BlockState *state;
            if (inner_pos.as_offset().y < 8) {
                state = Blocks::get().STONE->get_default_blockstate();
            } else {
                state = Blocks::get().AIR->get_default_blockstate();
            }
            chunk->set_block_state(inner_pos, state, SetBlockOption::DIRECT);
        }

        chunk->current_state = ChunkState::GENERATED;

        return chunk;
    }
};

} // namespace Craft