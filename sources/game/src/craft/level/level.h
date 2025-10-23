#pragma once

#include "ChunkGenerator.h"
#include "core/RefCount.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "craft/level/LevelRenderer.h"
#include "craft/level/RayCast.h"
#include "craft/level/chunk.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace Craft {

class Level : public RefCount {
public:
    using GameClock = std::chrono::steady_clock;
    static constexpr GameClock::duration TICK_INTERVAL = std::chrono::milliseconds(50); // 50 ms
private:
    Goonya::World *bind_world;

    ChunkGenerator chunk_generator;
    LevelRenderer level_renderer;

    std::unordered_map<ChunkPos, Ref<Chunk>> all_chunks;       // 所有（包括正在生成）的区块
    std::unordered_map<ChunkPos, Ref<Chunk>> accessible_chunk; // 所有可以被逻辑线程访问的区块

    GameClock::time_point last_real_frame_time;
    GameClock::duration delta_time_residual;
    GameTime world_time = 0; // 从开始运行的游戏刻数

    std::shared_ptr<Goonya::GObject> player;
    ChunkPos player_chunk_pos;

    int32_t chunk_load_distance = 6;

public:
    Level(Goonya::World *world, const std::shared_ptr<Goonya::GObject> &player);
    ~Level() {
        for (const auto &[_, c] : accessible_chunk) {
            level_renderer.unregister_chunk(c);
        }
    }

    Ref<Chunk> get_chunk(ChunkPos pos) const noexcept {
        auto iter = accessible_chunk.find(pos);
        return iter != all_chunks.end() ? iter->second : nullptr;
    }

    BlockState *get_block_state(BlockPos pos) const noexcept {
        Ref<Chunk> chunk = get_chunk(ChunkPos{pos});
        if (chunk == nullptr) {
            return Blocks::get().AIR->get_default_blockstate();
        }
        return chunk->get_block_state(BlockInnerPos{pos});
    }

    bool set_block_state(BlockPos pos, BlockState *state) noexcept // NOLINT
    {
        Ref<Chunk> chunk = get_chunk(ChunkPos{pos});
        if (chunk == nullptr) {
            return false;
        }
        BlockState* old = chunk->set_block_state(pos, state);
        if (old != state){
            level_renderer.notify_chunk_update(chunk);
        }
        return true;
    }

    BlockHitResult ray_cast(Ray ray, float max_distance) const noexcept;

    void prepare_start() {
        last_real_frame_time = GameClock::now();
        delta_time_residual = GameClock::duration::zero();
        assert(world_time == 0);
    }

    void tick();

private:
    void do_tick();
    void load_chunks();
};

} // namespace Craft