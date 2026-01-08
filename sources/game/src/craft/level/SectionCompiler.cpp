#include "SectionCompiler.h"

#include "core/ThreadPool.h"
#include "craft/block/block.h"
#include "craft/core/core.h"
#include "craft/level/CraftGraphicsBasic.h"
#include "craft/level/LevelRenderer.h"
#include "craft/model_manager.h"

#include <cstdint>

namespace Craft {

RenderChunkRegion RenderRegionCache::create_region(ChunkPos section_pos) {
    constexpr std::array<Vector3i, 7> pos_in_region{
        {{0, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};

    RenderChunkRegion region;
    region.center_chunk_pos = section_pos;

    for (Vector3i offset : pos_in_region) {
        ChunkPos pos{section_pos + offset};
        auto iter = cached_render_chunk.find(pos);
        if (iter != cached_render_chunk.end()) {
            region[pos] = iter->second;
        } else {
            RenderSection *section = level.get_section(pos);
            GN_ASSERT(section);
            auto render_chunk = std::make_shared<RenderChunk>(section->origin_chunk);
            cached_render_chunk.emplace(pos, render_chunk);

            region[pos] = render_chunk;
        }
    }

    return region;
}

void ComplieTask::do_complie() {
    if (is_cancelled.load(std::memory_order::acquire)) {
        return;
    }

    ComplieResult result = compile_mesh(pos);

    if (is_cancelled.load(std::memory_order::acquire)) {
        return; // 再检查一次
    }

    // 我们总是先取消旧任务，再启动新任务，然而这也不一定保证旧任务结束先于新任务结束
    Goonya::THREAD_POOL.enqueue_renderer_thread(
        [receiver = std::move(this->receiver), result = std::move(result), version = this->version] mutable {
            receiver(std::move(result), version);
        });
}

void ComplieTask::compiler_push_quad(ComplieResult &result, BlockState *state, BlockPos pos,
                                     const BakedQuad &quad) noexcept {
    Goonya::Vector3f normal = get_direction_vector(quad.normal);

    Goonya::Vector3f tint_color;
    if (quad.tintindex != -1) {
        tint_color = state->get_block()->get_tint_color(state, pos, quad.tintindex);
    } else {
        tint_color = {1.0f, 1.0f, 1.0f};
    }

    result.per_surface.emplace_back(TerrainPerSurface{
        .basecolor_id = quad.color_texture_index,
        .tint_color = tint_color,
        .normal = normal,
    });

    for (const auto &v : quad.vertices) {
        uint32_t base_index = result.vertices.size() * 4;

        Goonya::Vector3f world_pos = Goonya::Vector3f(pos.x, pos.y, pos.z) + v.position;
        result.vertices.emplace_back(world_pos, v.uv);

        result.indices.push_back(base_index + 0);
        result.indices.push_back(base_index + 1);
        result.indices.push_back(base_index + 2);
        result.indices.push_back(base_index + 2);
        result.indices.push_back(base_index + 3);
        result.indices.push_back(base_index + 0);
    }
}

ComplieResult ComplieTask::compile_mesh(ChunkPos pos) const {
    ComplieResult result;

    for (BlockPos pos : Vector3i::iterate_region(pos.get_start_pos(), pos.get_end_pos())) {
        BlockState *state = region.get_block_state(pos);
        const BakedBlockModel &model = ModelManager::get().get_baked_model(state, pos);
        // 在每个方向上根据是否被遮挡计算未被遮挡的面
        for (Direction direction : DIRECTION_VALUES) {
            BlockState *opposite = region.get_block_state(BlockPos{pos.move(direction)});
            bool hide = opposite->can_hide_face(direction_opposite(direction));
            if (hide) continue;

            for (const BakedQuad &quad : model.culled_quads[std::to_underlying(direction)]) {
                compiler_push_quad(result, state, pos, quad);
            }
        }
        // 增加永远不会被遮挡的面
        for (const BakedQuad &quad : model.unculled_quads) {
            compiler_push_quad(result, state, pos, quad);
        }
    }
    return result;
}
} // namespace Craft