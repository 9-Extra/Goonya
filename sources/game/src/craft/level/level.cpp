#include "level.h"
#include "craft/core/core.h"
#include "craft/level/chunk.h"
#include <ranges>

namespace Craft {

Level::Level() : delta_time_residual(GameClock::duration::zero()) {
    world_time = GameClock::time_point::min();

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

        world_time += TICK_INTERVAL;
    }

    last_real_frame_time = current_time;
    delta_time_residual -= contigues_tick * TICK_INTERVAL;
    if (delta_time_residual > TICK_INTERVAL) {
        LOG_WARN("Lagged: {} tick", delta_time_residual / TICK_INTERVAL);
    }
    delta_time_residual %= TICK_INTERVAL; // 延迟超过一帧则丢帧
}

void Level::load_chunks() {
    ChunkPos current_player_chunk_pos = ChunkPos(BlockPos(player_pos));
    if (current_player_chunk_pos == player_chunk_pos) {
        return;
    }
    player_chunk_pos = current_player_chunk_pos;

    for (int32_t x = player_chunk_pos.x - 8; x <= player_chunk_pos.y + 8; x++) {
        for (int32_t y = player_chunk_pos.x - 8; y <= player_chunk_pos.y + 8; y++) {
            for (int32_t z = player_chunk_pos.x - 8; z <= player_chunk_pos.y + 8; z++) {
                ChunkPos chunk_pos = ChunkPos{x, y, z};
                if (!all_chunks.contains(chunk_pos)) {
                    Ref<Chunk> chunk = all_chunks.emplace(chunk_pos, create_ref<Chunk>(chunk_pos)).first->second;

                    chunk_generator.process_chunk_async(chunk, generated_chunk_queue);
                }
            }
        }
    }
}

void Level::do_tick() {
    load_chunks();
    std::optional<Ref<Chunk>> chunk_opt = generated_chunk_queue.pop_front();
    while(chunk_opt.has_value()){
        ChunkPos pos = chunk_opt.value()->chunk_pos;

        // some postprocess here...

        visible_chunks.emplace(pos, std::move(chunk_opt.value()));

        chunk_opt = generated_chunk_queue.pop_front();
    }

    chunk_generator.tick();
}

} // namespace Craft