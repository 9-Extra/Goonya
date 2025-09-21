#pragma once

#include "ChunkGenerator.h"
#include "craft/core/core.h"
#include "core/RefCount.h"
#include "core/cgmath.h"
#include "craft/level/LevelRenderer.h"
#include "craft/level/chunk.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <future>
#include <unordered_map>
#include <vector>

namespace Craft{

class Level{
public:
    using GameClock = std::chrono::steady_clock;
    static constexpr GameClock::duration TICK_INTERVAL = std::chrono::milliseconds(50); // 50 ms
private:
    ChunkGenerator chunk_generator;
    ChunkPos player_chunk_pos;

    GameClock::time_point last_real_frame_time;
    GameClock::duration delta_time_residual;
    GameTime world_time = 0; // 从开始运行的游戏刻数

    std::unordered_map<ChunkPos, Ref<Chunk>> all_chunks; // 所有（包括正在生成）的区块
    std::unordered_map<ChunkPos, Ref<Chunk>> accessible_chunk; // 所有可以被逻辑线程访问的区块

    std::vector<std::future<Ref<Chunk>>> generating_chunks; // 正在生成的区块
    
    Goonya::Vector3f player_pos;

    LevelRenderer level_renderer;

    int32_t chunk_load_distance = 2;

public:
    Level();
    ~Level(){
        for(const auto& [_, c]: accessible_chunk){
            level_renderer.unregister_chunk(c);
        }
    }

    Ref<Chunk> get_chunk(ChunkPos pos) const noexcept{
        auto iter = accessible_chunk.find(pos);
        return iter != all_chunks.end() ? iter->second : nullptr;
    }

    void prepare_start(){
        last_real_frame_time = GameClock::now();
        delta_time_residual = GameClock::duration::zero();
        assert(world_time == 0);
    }

    void tick();

private:
    void do_tick();
    void load_chunks();
};

}