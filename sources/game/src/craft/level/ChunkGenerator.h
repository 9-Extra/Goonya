#pragma once

#include "chunk.h"
#include "core/ThreadPool.h"
#include "core/noise/PerlinNoise.h"
#include "craft/block/all_blocks.h"
#include "craft/core/core.h"
#include "runtime/GAssert.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <ranges>

namespace Craft {

struct ChunkProcessTask {
    std::future<void> blocker;
};

class ChunkGenerator {
private:
    int32_t terrain_height_base = 0;

    siv::BasicPerlinNoise<float> terrain_height_noise{42};
    std::vector<std::future<void>> running_tasks;
    std::atomic<bool> is_stopped = false;

public:
    ChunkGenerator() noexcept = default;
    ~ChunkGenerator() {
        GN_ASSERT(running_tasks.empty()); // 析构时，所有任务都应该已经完成
    }

    /**
     * @brief 将已完成的任务从running_tasks中移除，不阻塞
     */
    void suppress_running_tasks() {
        size_t valid_task_index = 0;
        for (auto &&[index, task] : std::views::enumerate(running_tasks)) {
            // 只有std::async创建的std::future会在析构时join，来自std::packaged_task的future则不会，需要手动等待
            if (task.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                running_tasks[valid_task_index] = std::move(task);
                valid_task_index++;
            }
        }
        running_tasks.resize(valid_task_index);
    }

    void wait_all_tasks() {
        is_stopped.store(true, std::memory_order::release);
        for (auto &&task : running_tasks) {
            task.wait();
        }
        running_tasks.clear();
    }

    void process_chunk_async(const Ref<Chunk> &chunk, std::move_only_function<void(const Ref<Chunk> &)> delegate) {
        running_tasks.emplace_back(
            Goonya::THREAD_POOL.enqueue([this, chunk = chunk, delegate = std::move(delegate)] mutable {
                if (is_stopped.load(std::memory_order::acquire)) {
                    return;
                }
                do_process_chunk(chunk);
                if (is_stopped.load(std::memory_order::acquire)) {
                    return;
                }
                Goonya::THREAD_POOL.enqueue_main_thread(
                    [chunk, delegate = std::move(delegate)] mutable { delegate(chunk); });
            }));
    }

private:
    int32_t get_terrain_height(int32_t x, int32_t z) const noexcept {
        const float frequence = 0.1f;
        return terrain_height_base + int32_t(terrain_height_noise.octave2D(x * frequence, z * frequence, 3) * 5);
    }

    void do_process_chunk(Ref<Chunk> chunk) const {
        GN_ASSERT(chunk->current_state == ChunkState::EMPTY);

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
                        if (world_pos.y == terrain_height_x_z) {
                            state = Blocks::get().GRASS_BLOCK->get_default_blockstate();
                        } else if (world_pos.y >= terrain_height_x_z - 3) {
                            state = Blocks::get().DIRT->get_default_blockstate();
                        } else {
                            state = Blocks::get().STONE->get_default_blockstate();
                        }
                    }
                    chunk->set_block_state(inner_pos, state);
                }
            }
        }

        chunk->current_state = ChunkState::GENERATED;
    }
};

} // namespace Craft