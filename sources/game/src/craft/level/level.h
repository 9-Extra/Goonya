#pragma once

#include "ChunkGenerator.h"
#include "core/RefCount.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "craft/level/LevelRenderer.h"
#include "craft/level/Player.h"
#include "craft/level/RayCast.h"
#include "craft/level/WireFrame.h"
#include "craft/level/chunk.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

#include <cstdint>
#include <unordered_map>

namespace Craft {

class Level : public RefCount {
private:
    ChunkGenerator chunk_generator;
    LevelRenderer level_renderer;

    std::unordered_map<ChunkPos, Ref<Chunk>> all_chunks;       // 所有（包括正在生成）的区块
    std::unordered_map<ChunkPos, Ref<Chunk>> accessible_chunk; // 所有可以被逻辑线程访问的区块

    Player player;
    bool is_breaking_block = false;
    bool is_placing_block = false;
    uint64_t last_break_block_tick = 0;
    constexpr static int8_t BLOCK_BREAK_INTERVAL = 2;
    WireFrame wire_frame;

    int32_t chunk_load_distance = 6;

public:
    Level(Goonya::World *world, const std::shared_ptr<Goonya::GObject> &player);
    ~Level() {
        for (const auto &[_, c] : accessible_chunk) {
            level_renderer.unregister_chunk(c->chunk_pos);
        }
    }

    Ref<Chunk> get_chunk(ChunkPos pos) const noexcept {
        auto iter = accessible_chunk.find(pos);
        return iter != all_chunks.end() ? iter->second : nullptr;
    }

    BlockState *get_block_state(BlockPos pos) const noexcept {
        Ref<Chunk> chunk = get_chunk(ChunkPos{pos});
        if (chunk == nullptr) {
            return nullptr;
        }
        return chunk->get_block_state(BlockInnerPos{pos});
    }

    bool set_block_state(BlockPos pos, BlockState *state) noexcept // NOLINT
    {
        Ref<Chunk> chunk = get_chunk(ChunkPos{pos});
        if (chunk == nullptr) {
            return false;
        }
        BlockState *old = chunk->set_block_state(pos, state);
        if (old != state) {
            level_renderer.notify_block_update(pos, state);
        }
        return true;
    }

    BlockHitResult ray_cast(Ray ray, float max_distance) const noexcept;

    void tick();
    void fixed_tick();

private:
    void load_chunks();
};

} // namespace Craft