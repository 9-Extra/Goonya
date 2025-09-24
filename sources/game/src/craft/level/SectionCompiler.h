#pragma once

#include "craft/block/block_model.h"
#include "craft/block/blockstate.h"
#include "craft/level/CraftGraphicsBasic.h"
#include "craft/level/chunk.h"
#include <future>
namespace Craft {

/**
 * @brief 一个区块信息的复制，用于在编译时访问
 * 与Minecraft不同，这里的一个区块就是一个section
 */
struct RenderChunk {
    std::array<BlockState *, CHUNK_BLOCK_COUNT> block_states;
    std::unordered_map<BlockInnerPos, int, BlockInnerPos::Hasher> block_entities;

    // 从Chunk复制数据
    explicit RenderChunk(Ref<Chunk> chunk) : block_states(chunk->block_states), block_entities(chunk->block_entities) {}

    BlockState *get_block_state(BlockInnerPos pos) const noexcept { return block_states[pos.as_index()]; }
    BlockState *get_block_state(BlockPos pos) const noexcept { return get_block_state(BlockInnerPos(pos)); }
};

/**
 * @brief 记录一个section编译时其自身及附近的6个区块信息
 *
 */
struct RenderChunkRegion {
    std::array<std::shared_ptr<RenderChunk>, 7> render_chunks;
    ChunkPos center_chunk_pos; // 角落处区块的位置

    BlockState *get_block_state(BlockPos pos) const noexcept {
        return render_chunks[flatten_index(center_chunk_pos, ChunkPos(pos))]->get_block_state(pos);
    }

    std::shared_ptr<RenderChunk> &operator[](ChunkPos pos) noexcept {
        return render_chunks[flatten_index(center_chunk_pos, pos)];
    }

    static size_t flatten_index(ChunkPos center_chunk_pos, ChunkPos pos) noexcept {
        Vector3i offset = pos - center_chunk_pos;
        assert(center_chunk_pos.distance_manhattan(pos) <= 1);
        if (offset.x != 0) {
            return offset.x == -1 ? 1 : 2;
        } else if (offset.y != 0) {
            return offset.y == -1 ? 3 : 4;
        } else if (offset.z != 0) {
            return offset.z == -1 ? 5 : 6;
        } else [[likely]] {
            return 0;
        }
    }
};

class LevelRenderer;

/**
 * @brief 缓存一帧中所有编译时需要的所有RenderChunk
 * 构建ComplieTask时从这里取出对应section需要的7个RenderChunk打包成RenderChunkRegion
 */
struct RenderRegionCache {
    const LevelRenderer &level;
    std::unordered_map<ChunkPos, std::shared_ptr<RenderChunk>> cached_render_chunk;

    explicit RenderRegionCache(const LevelRenderer &level) noexcept : level(level) {}

    RenderChunkRegion create_region(ChunkPos section_pos);
};

class RenderSection;

struct ComplieTask {
    std::weak_ptr<RenderSection> owner; // 持有owner，你才知道结果写到哪里去
    RenderChunkRegion region;
    std::atomic<bool> is_cancelled = false;
    std::future<void> task_blocker; // 用于等待此task结束

    ComplieTask(const std::weak_ptr<RenderSection> &owner, RenderChunkRegion region) noexcept
        : owner(owner), region(std::move(region)) {}

    ~ComplieTask() {
        assert(is_cancelled.load()); // 销毁任务前应当取消任务
        if (task_blocker.valid()) {
            task_blocker.wait(); // 如果被调度了，则等待任务结束，避免任务访问无效数据
        }
    }
    void cancel() noexcept { is_cancelled.store(true, std::memory_order::relaxed); }

    struct ComplieResult {
        std::vector<TerrainMeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<TerrainPerSurface> per_surface;
    };

    void do_complie(std::move_only_function<void(std::shared_ptr<RenderSection> &, ComplieTask::ComplieResult &&)>
                        &&delegate) const;

private:
    static void compiler_push_quad(ComplieResult &result, BlockPos pos, const BakedQuad &quad) noexcept;

    ComplieResult compile_mesh() const;
};

} // namespace Craft