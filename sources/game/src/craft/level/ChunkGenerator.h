#pragma once

#include "chunk.h"
#include "core/ThreadPool.h"
#include "core/noise/PerlinNoise.h"
#include "craft/block/all_blocks.h"
#include "craft/core/core.h"

#include <cstdint>
#include <functional>
#include <future>   

namespace Craft {

struct ChunkProcessTask {
    std::future<void> blocker;
};

class ChunkGenerator {
private:
    int32_t terrain_height_base = 0;

    siv::BasicPerlinNoise<float> terrain_height_noise{42};

public:
    ChunkGenerator() noexcept = default;
    ~ChunkGenerator() = default;

    void process_chunk_async(const Ref<Chunk> &chunk, std::move_only_function<void(const Ref<Chunk> &)> delegate) {
        Goonya::THREAD_POOL.enqueue_detached([this, chunk = chunk, delegate = std::move(delegate)] mutable {
            do_process_chunk(chunk);
            Goonya::THREAD_POOL.enqueue_main_thread(
                [chunk, delegate = std::move(delegate)] mutable { delegate(chunk); });
        });
    }

private:
    int32_t get_terrain_height(int32_t x, int32_t z) const noexcept {
        const float frequence = 0.1f;
        return terrain_height_base + terrain_height_noise.octave2D(x * frequence, z * frequence, 3) * 5;
    }

    void do_process_chunk(Ref<Chunk> chunk) const {
        assert(chunk->current_state == ChunkState::EMPTY);

        // LOG_TRACE("正在生成{}处的区块", chunk->chunk_pos);

        BlockPos start_pos = chunk->chunk_pos.get_start_pos();
        for (int32_t x = 0; x < (int32_t)CHUNK_WIDTH; x++) {
            for (int32_t z = 0; z < (int32_t)CHUNK_WIDTH; z++) {
                const int32_t terrain_height_x_z = get_terrain_height(start_pos.x + x, start_pos.z + z);
                for (int32_t y = 0; y < (int32_t)CHUNK_WIDTH; y++) {
                    BlockInnerPos inner_pos{BlockPos{x, y, z}};
                    BlockPos world_pos = BlockPos(chunk->chunk_pos.get_start_pos() + Vector3i{x, y, z});

                    BlockState *state;
                    if (world_pos.y > terrain_height_x_z) {
                        state = Blocks::get().AIR->get_default_blockstate();
                    } else {
                        if (world_pos.y == terrain_height_x_z){
                            state = Blocks::get().GRASS_BLOCK->get_default_blockstate();    
                        } else if (world_pos.y >= terrain_height_x_z - 3){
                            state = Blocks::get().DIRT->get_default_blockstate();    
                        } else {
                            state = Blocks::get().STONE->get_default_blockstate();
                        }
                    }
                    chunk->set_block_state(inner_pos, state, SetBlockOption::DIRECT);
                }
            }
        }

        chunk->current_state = ChunkState::GENERATED;
    }
};

} // namespace Craft