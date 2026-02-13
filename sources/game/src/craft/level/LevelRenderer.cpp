#include "LevelRenderer.h"

#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "craft/core/core.h"
#include "craft/level/CraftGraphicsBasic.h"
#include "craft/model_manager.h"
#include "function/renderer/Material.h"
#include "function/renderer/RScene.h"
#include "function/renderer/RendererBasic.h"
#include "function/renderer/UberShader.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/ResMng.h"

#include <cstdint>
#include <memory>
#include <span>

namespace Craft {

void RenderSection::complie_async(RenderRegionCache &region_cache, const Ref<Material> &terrain_material) {
    GN_ASSERT(is_dirty);

    auto receiver = [section_ptr = this->weak_from_this(), terrain_material = terrain_material](ComplieResult &&result,
                                                                                                uint32_t version) {
        ASSERT_RENDER_THREAD();
        std::shared_ptr<RenderSection> section = section_ptr.lock();
        if (section == nullptr) {
            return; // 区块已被销毁
        }
        if (version < section->version) {
            return; // 当前版本较旧，跳过
        }

        Goonya::RScene *scene = section->get_scene();
        if (scene == nullptr) {
            return; // 场景已经不存在
        }

        // ---------------开始更新----------------
        section->version = version; // 更新版本号

        // 没有物体则不需要网格
        if (result.indices.empty()) {
            if (section->mesh) {
                section->mesh.reset();
                section->mark_dirty(RenderSection::DirtyBit::Mesh);
            }
            return;
        }

        Goonya::Vector3f start_pos = section->chunk_pos.get_start_pos();
        Goonya::Vector3f end_pos = start_pos + Goonya::Vector3f{CHUNK_WIDTH, CHUNK_WIDTH, CHUNK_WIDTH};
        Goonya::BoundingBox bbox{start_pos, end_pos};

        if (!section->mesh) {
            section->mesh = create_ref<Goonya::GLMesh>(VERTEX_LAYOUT_PLANE);
            section->mesh->submeshes.resize(1);
        }
        section->mesh->submeshes[0] = Goonya::SubMesh{.start_index = 0,
                                                      .index_count = (uint32_t)result.indices.size(),
                                                      .topology = Goonya::Topology::TRIANGLE,
                                                      .aabb = bbox};

        section->mesh->set_vertices(0, std::as_bytes(std::span(result.vertices)));
        section->mesh->set_indices(result.indices);

        std::span<const std::byte> per_surface_data{std::as_bytes(std::span{result.per_surface})};
        Ref<Goonya::GLBuffer> updated_per_surface_buffer =
            create_ref<Goonya::GLBuffer>(Goonya::BufferType::DEVICE_ONLY, per_surface_data);

        // LOG_INFO("位于 {} 的区块编译完成", section->chunk_pos);
        if (section->materials.empty()) {
            // 没有材质则创建一个
            Ref<Material> material = terrain_material->clone();
            section->materials = {material};
        }

        section->materials[0]->set_external_buffer("per_surface", updated_per_surface_buffer);

        section->mark_dirty(DirtyBit::Init);
    };

    if (complie_task) {
        complie_task->cancel();
    }

    version++;
    // 提交编译任务
    complie_task = std::make_shared<ComplieTask>(chunk_pos, region_cache.create_region(chunk_pos), version, receiver);
    complie_task->launch();

    is_dirty = false;
}

LevelRenderer::LevelRenderer(Goonya::RScene *render_scene) : render_scene(render_scene) {
    Goonya::UberShader *shader = Goonya::resources.load_resource<Goonya::UberShader>(TERRAIN_SHADER_NAME).get();
    terrain_material = create_ref<Material>(shader);
    terrain_material->set_texture("basecolor_texture", ModelManager::get().get_textures());
}

void LevelRenderer::render_frame() {
    do_cull();

    // 提交所有需要编译的区块
    RenderRegionCache region_cache{*this};
    for (RenderSection *section : visible_chunk) {
        if (!section->is_dirty) continue;
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
