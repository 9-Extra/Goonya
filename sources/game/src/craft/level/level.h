#pragma once

#include "ChunkGenerator.h"
#include "craft/core/LockQueue.h"
#include "craft/core/core.h"
#include "core/RefCount.h"
#include "core/cgmath.h"
#include "craft/level/chunk.h"

#include <cassert>
#include <chrono>
#include <unordered_map>

namespace Craft{

class Level{
public:
    using GameClock = std::chrono::steady_clock;
    static constexpr GameClock::duration TICK_INTERVAL = std::chrono::milliseconds(50); // 50 ms
private:
    ChunkGenerator chunk_generator;
    ChunkPos player_chunk_pos;

    GameClock::time_point last_real_frame_time;
    GameClock::time_point world_time;
    GameClock::duration delta_time_residual;

    std::unordered_map<ChunkPos, Ref<Chunk>> all_chunks; // 所有（包括正在生成）的区块
    std::unordered_map<ChunkPos, Ref<Chunk>> visible_chunks; // 所有可以被逻辑线程访问的区块

    LockQueue<Ref<Chunk>> generated_chunk_queue; // 刚刚完成生成的区块
    
    Goonya::Vector3f player_pos;


public:
    Level();

    void prepare_start(){
        last_real_frame_time = GameClock::now();
    }

    void tick();

private:
    void do_tick();
    void load_chunks();
};

}