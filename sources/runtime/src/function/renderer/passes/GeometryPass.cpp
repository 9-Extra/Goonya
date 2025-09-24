#include "GeometryPass.h"
#include "core/cgmath.h"
#include "core/log/Log.h"
#include "function/renderer/Renderer.h"
#include "platform/graphics/Graphics.h"
#include "resource/ResMng.h"
#include "core/timer/timer.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>

namespace Goonya::Graphics {

GeometryPass::GeometryPass() {
    per_frame_uniform = graphics_api->create_buffer(sizeof(PerFrameData), BufferType::DYNAMIC);
    per_frame_uniform->set_debug_label("Lambert Per Frame");
}
// 一般物体渲染
void GeometryPass::run() {
    const CameraRenderProxy *camera = renderer.current_camera;
    Vector3f camera_pos = camera->get_position();

    // 绑定per_frame uniform buffer
    per_frame_uniform->bind_uniform(0);

    {
        // 填充per_frame uniform数据
        StructBufferWriter<PerFrameData> data(per_frame_uniform, BufferMapOption::WRITE_DISCARD);
        // 透视投影矩阵
        data->view_perspective_matrix = camera->get_view_perspective_matrix().transpose();
        // 相机位置
        data->camera_position = camera_pos;
        // 雾参数
        assert(renderer.fog_density >= 0.0f);
        data->fog_density = renderer.fog_density;
        data->fog_min_distance = renderer.fog_min_distance;
        data->time = Timer::total();
        // 灯光参数
        data->ambient_light = renderer.ambient_light;
        if (renderer.pointlights.size() > POINTLIGHT_MAX) {
            LOG_WARN("点光源数量({})超出上限({})", renderer.pointlights.size(), POINTLIGHT_MAX);
        }
        uint32_t count = static_cast<uint32_t>(std::min<size_t>(renderer.pointlights.size(), POINTLIGHT_MAX));
        for (uint32_t i = 0; i < count; ++i) {
            data->pointlight_list[i].position = renderer.pointlights[i].position;
            data->pointlight_list[i].intensity = renderer.pointlights[i].color * renderer.pointlights[i].factor;
        }
        data->pointlight_num = count;
        // 填充结束
    }

    Ref<Material> default_material = resources.materials.get("materials/default");

    struct alignas(256) PerObjectData final {
        Matrix4 model_matrix;
        Matrix4 normal_matrix; // 内存对齐
    };

    struct Batch { // NOLINT
        const Mesh *mesh;
        SubMesh sub_mesh;
        size_t per_object_data_offset;
    };

    std::unordered_map<Material *, std::vector<Batch>> batcher;

    // 把所有用于一般渲染每帧变化的数据收集到一个buffer中
    Ref<Buffer> per_object_uniform =
        graphics_api->create_buffer(renderer.mesh_proxys.size() * sizeof(PerObjectData), BufferType::STREAM);
    per_object_uniform->set_debug_label("Lambert Per Object");

    {
        ArrayBufferWriter<PerObjectData> per_object_data(per_object_uniform, BufferMapOption::WRITE_DISCARD);

        // 遍历所有part，绘制每一个part
        for (const auto [offset, mesh] : std::views::enumerate(renderer.mesh_proxys)) {
            const Mesh *m = mesh->mesh.get();

            // 填充PerObject参数
            per_object_data[offset]->model_matrix = mesh->model_matrix.transpose();
            per_object_data[offset]->normal_matrix = Matrix4{mesh->normal_matrix.transpose()};

            for (uint32_t i = 0; i < m->submeshes.size(); i++) {
                if (m->submeshes[i].index_count == 0){
                    continue;
                }
                
                // 材质未设置时使用默认材质，多出来则无视
                auto current_material = i < mesh->materials.size() ? mesh->materials[i] : default_material;

                Batch batch{m, m->submeshes[i], offset * sizeof(PerObjectData)};

                batcher[current_material.get()].emplace_back(batch);
            }
        }
    }

    for (auto &[material, batch] : batcher) {
        material->bind();

        for (Batch &item : batch) {
            per_object_uniform->bind_uniform_ranged(1, item.per_object_data_offset, sizeof(PerObjectData));
            item.mesh->bind();
            graphics_api->draw_submesh(item.sub_mesh);
        }
    }
}

} // namespace Goonya::Graphics