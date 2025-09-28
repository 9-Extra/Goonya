#include "LevelRenderer.h"

#include "core/RefCount.h"
#include "core/cgmath.h"
#include "core/log/Log.h"
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
#include <span>

namespace Craft {

LevelRenderer::LevelRenderer() {
    Goonya::Graphics::UberShader *shader = Goonya::resources.shader_lib->query_uber_shader(TERRAIN_SHADER_NAME);
    terrain_material = create_ref<Goonya::Graphics::Material>(shader);
    terrain_material->set_pipeline_state(Goonya::Graphics::PipeLineState{
        .depth_test = Goonya::Graphics::DepthTestMode::LESS,
        .cull_mode = Goonya::Graphics::CullFaceMode::BACK,
    });
    terrain_material->set_texture("basecolor_texture", ModelManager::get().get_textures());
}

void LevelRenderer::render_frame() {
    do_cull();

    RenderRegionCache region_cache{*this};

    auto receiver = [terrain_material = this->terrain_material](std::shared_ptr<RenderSection> &section,
                                                                ComplieTask::ComplieResult &&result) {
        ASSERT_RENDER_THREAD();
        using namespace Goonya::Graphics;
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

        LOG_INFO("位于 {} 的区块编译完成", section->chunk_pos);

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

            renderer.add_mesh_proxy(std::unique_ptr<MeshRenderProxy>{proxy});
        } else {
            MeshRenderProxy *proxy = section->mesh_proxy;
            proxy->mesh = updated_mesh;
            proxy->materials[0]->set_external_buffer("per_surface", updated_per_surface_buffer);
        }
    };

    for (RenderSection *section : visible_chunk) {
        if (!section->is_dirty)
            continue;

        section->complie_async(region_cache, receiver);
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
} // namespace Craft