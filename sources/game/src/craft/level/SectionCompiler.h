#pragma once

#include "craft/block/block_model.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "craft/level/CraftGraphicsBasic.h"
#include "craft/level/chunk.h"
#include "craft/level/chunk_block_storage.h"
#include <cstdint>

namespace Craft {

/**
 * @brief 一个区块信息的复制，用于在编译时访问
 * 与Minecraft不同，这里的一个区块就是一个section
 */
struct RenderChunk {
    ChunkBlockStorage block_storage;
    std::unordered_map<BlockInnerPos, int, BlockInnerPos::Hasher> block_entities;

    // 从Chunk复制数据
    explicit RenderChunk(Ref<Chunk> chunk)
        : block_storage(chunk->block_storage), block_entities(chunk->block_entities) {}

    BlockState *get_block_state(BlockInnerPos pos) const noexcept {
        return block_storage.get_block_state(pos.as_index());
    }
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
        return render_chunks[flatten_index(center_chunk_pos, ChunkPos{pos})]->get_block_state(pos);
    }

    std::shared_ptr<RenderChunk> &operator[](ChunkPos pos) noexcept {
        return render_chunks[flatten_index(center_chunk_pos, pos)];
    }

    static size_t flatten_index(ChunkPos center_chunk_pos, ChunkPos pos) noexcept {
        Vector3i offset = pos - center_chunk_pos;
        GN_ASSERT(center_chunk_pos.distance_manhattan(pos) <= 1);
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

struct ComplieResult {
    std::vector<TerrainMeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<TerrainPerSurface> per_surface;
};

struct ComplieTask : public std::enable_shared_from_this<ComplieTask> {
private:
    ChunkPos pos; // 编译的区块位置
    RenderChunkRegion region;
    uint32_t version; // 编译版本
    std::atomic<bool> is_cancelled = false;
    std::move_only_function<void(ComplieResult &&, uint32_t)> receiver;

    bool is_launched = false;

public:
    ComplieTask(ChunkPos pos, RenderChunkRegion region, uint32_t version,
                std::move_only_function<void(ComplieResult &&, uint32_t)> receiver) noexcept
        : pos(pos), region(std::move(region)), version(version), receiver(std::move(receiver)) {}

    void cancel() noexcept { is_cancelled.store(true, std::memory_order::relaxed); }

    void launch() noexcept;

private:
    void do_compile();

    static void compiler_push_quad(ComplieResult &result, BlockState *state, BlockPos pos,
                                   const BakedQuad &quad) noexcept;

    ComplieResult compile_mesh(ChunkPos pos) const;
};

} // namespace Craft