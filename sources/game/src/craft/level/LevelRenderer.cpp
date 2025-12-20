#include "LevelRenderer.h"

#include "core/RefCount.h"
#include "craft/core/core.h"
#include "craft/level/CraftGraphicsBasic.h"
#include "craft/model_manager.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RendererBasic.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/UberShader.h"
#include "resource/ResMng.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <span>

namespace Craft {

void RenderSection::complie_async(RenderRegionCache &region_cache,
                                  const Ref<Goonya::Graphics::Material> &terrain_material) {
    assert(is_dirty);

    auto receiver = [section_ptr = this->weak_from_this(), &render_scene = render_scene,
                     terrain_material = terrain_material](ComplieResult &&result, uint32_t version) {
        ASSERT_RENDER_THREAD();

        using namespace Goonya::Graphics;
        std::shared_ptr<RenderSection> section = section_ptr.lock();
        if (section == nullptr) {
            return; // 区块已被销毁
        }
        if (version < section->version) {
            return; // 当前版本较旧，跳过
        }
        section->version = version; // 更新版本号
        if (result.indices.size() == 0) {
            // 对于没有东西需要渲染的区块，则其mesh_proxy都不需要存在
            if (section->mesh_proxy) {
                auto iter = render_scene.mesh_proxys.find(section->mesh_proxy);
                assert(iter != render_scene.mesh_proxys.end());
                render_scene.mesh_proxys.erase(iter);
                section->mesh_proxy = nullptr;
            }
            return;
        }

        Ref<Mesh> updated_mesh = graphics_api->create_mesh(VERTEX_LAYOUT_PLANE);
        updated_mesh->submeshes.emplace_back(SubMesh{.start_index = 0,
                                                     .index_count = (uint32_t)result.indices.size(),
                                                     .topology = Goonya::Graphics::Topology::TRIANGLE});

        updated_mesh->set_vertices(0, std::as_bytes(std::span(result.vertices)));
        updated_mesh->set_indices(result.indices);

        std::span<const std::byte> per_surface_data{std::as_bytes(std::span{result.per_surface})};
        Ref<Buffer> updated_per_surface_buffer =
            graphics_api->create_buffer(per_surface_data.size_bytes(), BufferType::STATIC);
        updated_per_surface_buffer->write(per_surface_data, 0);

        // LOG_INFO("位于 {} 的区块编译完成", section->chunk_pos);
        if (section->mesh_proxy == nullptr) {
            Ref<Material> material = terrain_material->clone();
            material->set_external_buffer("per_surface", updated_per_surface_buffer);

            Goonya::Vector3f start_pos = section->chunk_pos.get_start_pos();
            Goonya::Vector3f end_pos = start_pos + Goonya::Vector3f{CHUNK_WIDTH, CHUNK_WIDTH, CHUNK_WIDTH};

            MeshRenderProxy *proxy = new MeshRenderProxy{};
            proxy->mesh = updated_mesh;
            proxy->materials = {material};
            proxy->model_matrix = Goonya::Matrix4::identity();
            proxy->normal_matrix = Goonya::Matrix3::identity();
            proxy->aabbs = {Goonya::BoundingBox{start_pos, end_pos}};

            section->mesh_proxy = proxy;

            render_scene.mesh_proxys.emplace(std::unique_ptr<MeshRenderProxy>{proxy});
        } else {
            MeshRenderProxy *proxy = section->mesh_proxy;
            proxy->mesh = updated_mesh;
            proxy->materials[0]->set_external_buffer("per_surface", updated_per_surface_buffer);
        }
    };

    if (complie_task) {
        complie_task->cancel();
    }

    version++;
    complie_task = std::make_shared<ComplieTask>(chunk_pos, region_cache.create_region(chunk_pos), version, receiver);
    Goonya::THREAD_POOL.enqueue_detached([task = this->complie_task, receiver = std::move(receiver)] mutable {
        // 在LevelRenderer销毁前一定提前结束所有任务，所以queue一定可用
        task->do_complie();
    });

    is_dirty = false;
}

LevelRenderer::LevelRenderer(Goonya::Graphics::RenderScene &render_scene) : render_scene(render_scene) {
    Goonya::Graphics::UberShader *shader = Goonya::resources.load_resource<Goonya::Graphics::UberShader>(TERRAIN_SHADER_NAME).get();
    terrain_material = create_ref<Goonya::Graphics::Material>(shader);
    terrain_material->set_texture("basecolor_texture", ModelManager::get().get_textures());
}

void LevelRenderer::render_frame() {
    do_cull();

    // 提交所有需要编译的区块
    RenderRegionCache region_cache{*this};
    for (RenderSection *section : visible_chunk) {
        if (!section->is_dirty)
            continue;
        section->complie_async(region_cache, terrain_material);
    }
}

void LevelRenderer::do_cull() {
    visible_chunk.clear();
    for (const auto &[pos, section] : render_chunks) {
        if (has_all_neighbors(pos)) {
            visible_chunk.push_back(section.get());
        }
    }
}

void LevelRenderer::notify_block_update(BlockPos pos, BlockState *state) noexcept // NOLINT
{
    ChunkPos chunk_pos{pos};
    if (auto current_section = get_section(chunk_pos)) {

        // 更新当前的section
        current_section->is_dirty = true;

        // 如果方块位于区块边缘，则有可能需要更新旁边的区块
        BlockPos inner_pos = BlockInnerPos{pos}.as_offset();
        if (inner_pos.x == 0 && !state->can_hide_face(Direction::WEST)) {
            if (auto section = get_section(chunk_pos.move(Direction::WEST))) {
                section->is_dirty = true;
            }
        }
        if (inner_pos.x == CHUNK_WIDTH - 1 && !state->can_hide_face(Direction::EAST)) {
            if (auto section = get_section(chunk_pos.move(Direction::EAST))) {
                section->is_dirty = true;
            }
        }
        if (inner_pos.y == 0 && !state->can_hide_face(Direction::DOWN)) {
            if (auto section = get_section(chunk_pos.move(Direction::DOWN))) {
                section->is_dirty = true;
            }
        }
        if (inner_pos.y == CHUNK_WIDTH - 1 && !state->can_hide_face(Direction::UP)) {
            if (auto section = get_section(chunk_pos.move(Direction::UP))) {
                section->is_dirty = true;
            }
        }
        if (inner_pos.z == 0 && !state->can_hide_face(Direction::NORTH)) {
            if (auto section = get_section(chunk_pos.move(Direction::NORTH))) {
                section->is_dirty = true;
            }
        }
        if (inner_pos.z == CHUNK_WIDTH - 1 && !state->can_hide_face(Direction::SOUTH)) {
            if (auto section = get_section(chunk_pos.move(Direction::SOUTH))) {
                section->is_dirty = true;
            }
        }
    }
}
} // namespace Craft