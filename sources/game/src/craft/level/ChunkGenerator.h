#pragma once

#include "core/ThreadPool.h"
#include "chunk.h"
#include "craft/block/all_blocks.h"
#include "craft/core/LockQueue.h"

#include <future>

namespace Craft {

class ChunkGenerator {
private:
    std::vector<std::future<void>> chunks_processing;
    std::mutex mutex;

    int32_t air_layer = 60;

public:
    ChunkGenerator() noexcept = default;
    ~ChunkGenerator() {
        chunks_processing.clear(); // 优先析构，futurn会阻塞
    }

    void process_chunk_async(const Ref<Chunk> &chunk, LockQueue<Ref<Chunk>>& receiver) {
        std::lock_guard lock(mutex);
        chunks_processing.push_back(Goonya::THREAD_POOL.enqueue([this, chunk, &receiver] { 
             do_process_chunk(chunk); 
             receiver.push_back(chunk);
        }));
    }

    void tick() {
        size_t j = 0;
        std::lock_guard lock(mutex);
        for (std::future<void> &task : chunks_processing) {
            if (task.valid()) {
                // nothing to do?
            } else {
                chunks_processing[j] = std::move(task);
                j++;
            }
        }
        chunks_processing.resize(j);
    }

private:
    void do_process_chunk(Ref<Chunk> chunk) const {
        assert(chunk->current_state == ChunkState::EMPTY);

        // LOG_TRACE("正在生成{}处的区块", chunk->chunk_pos);

        for (uint32_t i = 0; i < CHUNK_BLOCK_COUNT; i++) {
            BlockInnerPos inner_pos{i};
            BlockPos world_pos = BlockPos(chunk->chunk_pos.get_start_pos() + inner_pos.as_offset());
            BlockState *state;
            if (world_pos.y < air_layer) {
                state = Blocks::get().STONE->get_default_blockstate();
            } else {
                state = Blocks::get().AIR->get_default_blockstate();
            }
            chunk->set_block_state(inner_pos, state, SetBlockOption::DIRECT);
        }

        chunk->current_state = ChunkState::GENERATED;
    }
};

} // namespace Craft