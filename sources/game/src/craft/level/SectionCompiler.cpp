#include "SectionCompiler.h"

#include "core/ThreadPool.h"
#include "craft/level/LevelRenderer.h"
#include "craft/model_manager.h"

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
            assert(section);
            auto render_chunk = std::make_shared<RenderChunk>(section->origin_chunk);
            cached_render_chunk.emplace(pos, render_chunk);

            region[pos] = render_chunk;
        }
    }

    return region;
}

void ComplieTask::do_complie(
    std::move_only_function<void(std::shared_ptr<RenderSection> &, ComplieResult &&)> &&delegate) const {
    std::shared_ptr<RenderSection> section = owner.lock();
    if (!section || is_cancelled.load(std::memory_order::acquire)) {
        return; // 被删了，不用看了
    }

    ComplieResult result = compile_mesh();

    if (is_cancelled.load(std::memory_order::acquire)) {
        return; // 再检查一次
    }

    Goonya::THREAD_POOL.enqueue_renderer_thread(
        [delegate = std::move(delegate), section, result = std::move(result)] mutable {
            delegate(section, std::move(result));
        });
}

void ComplieTask::compiler_push_quad(ComplieResult &result, BlockPos pos, const BakedQuad &quad) noexcept {
    Goonya::Vector3f normal = get_direction_vector(quad.normal);

    result.per_surface.emplace_back(quad.color_texture_index, normal);

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

ComplieTask::ComplieResult ComplieTask::compile_mesh() const {
    ComplieResult result;

    RenderSection *section = owner.lock().get();
    BlockPos origin = section->chunk_pos.get_start_pos();
    BlockPos end = section->chunk_pos.get_end_pos();
    for (int32_t x = origin.x; x < end.x; x++) {
        for (int32_t y = origin.y; y < end.y; y++) {
            for (int32_t z = origin.z; z < end.z; z++) {
                BlockPos pos{x, y, z};
                BlockState *state = region.get_block_state(pos);
                const BakedBlockModel &model = ModelManager::get().get_baked_model(state);
                // 在每个方向上根据是否被遮挡计算未被遮挡的面
                for (Direction direction : DIRECTION_VALUES) {
                    BlockState *opposite = region.get_block_state(BlockPos{pos.move(direction)});
                    bool hide = opposite->can_hide_face(direction_opposite(direction));
                    if (hide)
                        continue;

                    for (const BakedQuad &quad : model.culled_quads[std::to_underlying(direction)]) {
                        compiler_push_quad(result, pos, quad);
                    }
                }
                // 增加永远不会被遮挡的面
                for (const BakedQuad &quad : model.unculled_quads) {
                    compiler_push_quad(result, pos, quad);
                }
            }
        }
    }
    return result;
}
} // namespace Craft