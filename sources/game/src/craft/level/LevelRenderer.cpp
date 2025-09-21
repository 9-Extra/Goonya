#include "LevelRenderer.h"

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "craft/level/CraftGraphicsBasic.h"
#include "craft/model_manager.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/Renderer.h"
#include "function/renderer/RendererBasic.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include <cassert>
#include <cstdint>
#include <memory>

namespace Craft {

constexpr std::array<Vector3i, 7> pos_in_region{
    {{0, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};

RenderChunkRegion RenderRegionCache::create_region(ChunkPos section_pos) {
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
void RenderSection::ComplieTask::do_complie(
    LockQueue<std::move_only_function<void(LevelRenderer *)>> &update_tasks_queue) const {
    std::shared_ptr<RenderSection> section = owner.lock();
    if (!section || is_cancelled.load(std::memory_order::acquire)) {
        return; // 被删了，不用看了
    }

    ComplieResult result = compile_mesh();

    if (is_cancelled.load(std::memory_order::acquire)) {
        return; // 再检查一次
    }

    update_tasks_queue.push_back([result = std::move(result), section](LevelRenderer *level) {
        ASSERT_RENDER_THREAD();
        using namespace Goonya::Graphics;
        intrusive_ptr<Mesh> updated_mesh = graphics_api->create_mesh();
        updated_mesh->set_layout(VERTEX_LAYOUT_PLANE);
        updated_mesh->submeshes.emplace_back(SubMesh{.start_index = 0,
                                                     .index_count = (uint32_t)result.indices.size(),
                                                     .topology = Goonya::Graphics::Topology::TRIANGLE});

        intrusive_ptr<Buffer> vertex_buffer = graphics_api->create_buffer(result.vertices.size() * sizeof(TerrainMeshVertex), BufferType::STATIC);
        vertex_buffer->write(std::as_bytes(std::span(result.vertices)), 0);
        updated_mesh->set_vertex_buffer(vertex_buffer);

        intrusive_ptr<Buffer> index_buffer =
            graphics_api->create_buffer(result.indices.size() * sizeof(uint32_t), BufferType::STATIC);
        index_buffer->write(std::as_bytes(std::span(result.indices)), 0);
        updated_mesh->set_indices_buffer(index_buffer);

        std::span<const std::byte> per_surface_data{std::as_bytes(std::span{result.per_surface})};
        intrusive_ptr<Buffer> updated_per_surface_buffer =
            graphics_api->create_buffer(per_surface_data.size_bytes(), BufferType::STATIC);
        updated_per_surface_buffer->write(per_surface_data, 0);

        LOG_INFO("位于 {} 的区块编译完成", section->chunk_pos);

        if (section->mesh_proxy == nullptr) {
            intrusive_ptr<Material> material = level->terrain_material->clone();
            material->set_external_buffer("per_surface", updated_per_surface_buffer);

            MeshRenderProxy *proxy = new MeshRenderProxy{};
            proxy->mesh = updated_mesh;
            proxy->materials = {material};
            proxy->model_matrix = Goonya::Matrix4::identity();
            proxy->normal_matrix = Goonya::Matrix3::identity();

            section->mesh_proxy = proxy;

            enqueue_render_task([proxy = proxy] mutable {
                ASSERT_RENDER_THREAD();
                renderer.add_mesh_proxy(std::unique_ptr<MeshRenderProxy>{proxy});
            });
        } else {
            MeshRenderProxy *proxy = section->mesh_proxy;
            enqueue_render_task([proxy = proxy, mesh = updated_mesh, buffer = updated_per_surface_buffer] mutable {
                ASSERT_RENDER_THREAD();
                proxy->mesh = mesh;
                proxy->materials[0]->set_external_buffer("per_surface", buffer);
            });
        }
    });
}

void RenderSection::ComplieTask::compiler_push_quad(ComplieResult &result, BlockPos pos, const BakedQuad &quad) noexcept {
    Goonya::Vector3f normal = get_direction_vector(quad.normal);

    for (const auto &v : quad.vertices) {
        uint32_t base_index = result.vertices.size() * 4;

        Goonya::Vector3f world_pos = Goonya::Vector3f(pos.x, pos.y, pos.z) + v.position;
        result.vertices.emplace_back(world_pos, v.uv);

        result.per_surface.emplace_back(quad.color_texture_index, normal);

        result.indices.push_back(base_index + 0);
        result.indices.push_back(base_index + 1);
        result.indices.push_back(base_index + 2);
        result.indices.push_back(base_index + 2);
        result.indices.push_back(base_index + 3);
        result.indices.push_back(base_index + 0);
    }
}

RenderSection::ComplieTask::ComplieResult RenderSection::ComplieTask::compile_mesh() const {
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
                    BlockState* opposite = region.get_block_state(BlockPos{pos.move(direction)});
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
    LOG_WARN("Count = {}", result.indices.size());
    return result;
}

// -------------------------LevelRenderer----------------------------

LevelRenderer::LevelRenderer() {
    Goonya::Graphics::UberShader *shader = Goonya::resources.shader_lib->query_uber_shader(TERRAIN_SHADER_NAME);
    terrain_material = make_intrusive<Goonya::Graphics::Material>(shader);
    terrain_material->set_pipeline_state(Goonya::Graphics::PipeLineState{
        .depth_test = Goonya::Graphics::DepthTestMode::LESS,
        .cull_mode = Goonya::Graphics::CullFaceMode::BACK,
    });
    terrain_material->set_texture("basecolor_texture", ModelManager::get().get_textures());
}

void LevelRenderer::render_frame() {

    pull_tasks();
    do_cull();

    RenderRegionCache region_cache{*this};

    for (RenderSection *section : visible_chunk) {
        if (!section->is_dirty)
            continue;

        section->complie_async(region_cache, update_tasks);
    }
}

} // namespace Craft