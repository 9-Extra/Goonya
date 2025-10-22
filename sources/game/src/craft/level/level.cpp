#include "level.h"

#include "core/log/Log.h"
#include "craft/core/core.h"
#include "craft/level/LevelRenderer.h"
#include "craft/level/chunk.h"
#include "function/world/World.h"
#include <cassert>
#include <ranges>

namespace Craft {

Level::Level(Goonya::World* world) : bind_world(world), level_renderer(world->main_scene()), delta_time_residual(GameClock::duration::zero()) {
    assert(world != nullptr);
    player_pos = {0, 0, 0};
    player_chunk_pos = {255, 255, 255}; // 保证第一帧 ChunkPos(BlockPos(player_pos)) != play_chunk_pos
}

void Level::tick() {
    const GameClock::time_point current_time = GameClock::now();
    delta_time_residual += current_time - last_real_frame_time;
    uint32_t contigues_tick = std::min<uint32_t>(10, delta_time_residual / TICK_INTERVAL);

    for (auto _ : std::views::iota(uint32_t(0), contigues_tick)) {

        // ---------------------------
        do_tick(); // 一次tick
        // ---------------------------

        world_time++;
    }

    last_real_frame_time = current_time;
    delta_time_residual -= contigues_tick * TICK_INTERVAL;
    if (delta_time_residual >= TICK_INTERVAL) {
        LOG_WARN("Lagged: {} tick", delta_time_residual / TICK_INTERVAL);
        delta_time_residual %= TICK_INTERVAL; // 延迟超过一帧则丢帧
    }
}

void Level::load_chunks() {

    ChunkPos current_player_chunk_pos = ChunkPos(BlockPos(player_pos));
    if (current_player_chunk_pos == player_chunk_pos) {
        return;
    } else {
        player_chunk_pos = current_player_chunk_pos;
        auto processed_chunk_receiver = [level = Ref{this}](const Ref<Chunk> &chunk) mutable {
            level->accessible_chunk.emplace(chunk->chunk_pos, chunk);
            level->level_renderer.register_chunk(chunk);
        };
        // 添加区块加载任务
        for (int32_t x = player_chunk_pos.x - chunk_load_distance; x <= player_chunk_pos.x + chunk_load_distance; x++) {
            for (int32_t y = player_chunk_pos.y - chunk_load_distance; y <= player_chunk_pos.y + chunk_load_distance;
                 y++) {
                for (int32_t z = player_chunk_pos.z - chunk_load_distance;
                     z <= player_chunk_pos.z + chunk_load_distance; z++) {
                    ChunkPos chunk_pos = ChunkPos{x, y, z};
                    if (!all_chunks.contains(chunk_pos)) {
                        Ref<Chunk> chunk = all_chunks.emplace(chunk_pos, create_ref<Chunk>(chunk_pos)).first->second;
                        chunk_generator.process_chunk_async(chunk, processed_chunk_receiver);
                    }
                }
            }
        }
    }

    // 收集加载完成的区块到visible_chunks
}

void Level::do_tick() {
    load_chunks();

    LOG_INFO("{} {}",BlockPos{player_pos}, get_block_state(BlockPos{player_pos})->get_block()->get_display_name());

    level_renderer.render_frame();
}

} // namespace Craft