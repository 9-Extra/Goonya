#include "GeometryPass.h"
#include "core/cgmath.h"
#include "core/log/Log.h"
#include "core/timer/timer.h"
#include "function/renderer/Renderer.h"
#include "platform/graphics/Graphics.h"
#include "resource/ResMng.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>

namespace Goonya::Graphics {

GeometryPass::GeometryPass() {
    per_frame_uniform = graphics_api->create_buffer(sizeof(PerFrameData), BufferType::DYNAMIC);
    per_frame_uniform->set_debug_label("Lambert Per Frame");
}

/**
 * @brief 从矩阵重建视锥体
 *
 * @param mat 任意可以投影到OpenGL裁剪空间的矩阵（乘在右边的版本，Y轴翻转没有实际影响）
 * @return 视锥体的6个平面，法线向视椎体内，没有归一化
 */
std::array<Plane, 6> create_frustum_planes(const Matrix4 &mat) noexcept {
    Matrix4 col = mat.transpose();

    Plane p0 = Plane{col[3] + col[0]};
    Plane p1 = Plane{col[3] - col[0]};
    Plane p2 = Plane{col[3] + col[1]};
    Plane p3 = Plane{col[3] - col[1]};
    Plane p4 = Plane{col[3] + col[2]};
    Plane p5 = Plane{col[3] - col[2]};

    return {p0, p1, p2, p3, p4, p5};
}

/**
 * @brief 判断AABB和视锥体（6个平面）是否相交
 *
 * @param frustum 视锥体的6个平面，法线向视椎体内
 * @param aabb 包围盒
 */
bool intersect_frustum_aabb(const std::array<Plane, 6> &frustum, const BoundingBox &aabb) noexcept {
    // 计算AABB的中心点和半长（半宽高）
    Vector3f center = aabb.center();
    Vector3f extents = aabb.max - center; // 一半长度

    // 对每个视锥体平面进行测试
    for (int i = 0; i < 6; i++) {
        const Plane &p = frustum[i];

        Vector3f extent_distance = p.normal * extents;
        float r = std::abs(extent_distance.x) + std::abs(extent_distance.y) + std::abs(extent_distance.z);

        // 计算中心点到平面的距离
        float distance = p.normal.dot(center) + p.d;

        // distance < 0说明中心点在视椎体外
        // 而distance + r < 0说明AABB所有顶点中最靠近平面内部的顶点也在视椎体外，可以剔除
        // 如果使用distance - r < 0，则只会保留整个包围盒都在视锥体内部的物体
        if (distance + r < 0) {
            return false;
        }
    }

    // 如果所有平面测试都通过，则AABB与视锥体相交或在其内部
    return true;
}
// 一般物体渲染
void GeometryPass::run(CameraRenderProxy *camera) {
    Vector3f camera_pos = camera->get_position();

    const Matrix4 view_perspective = camera->get_view_perspective_matrix();

    // 绑定per_frame uniform buffer
    per_frame_uniform->bind_uniform(0);
    {
        // 填充per_frame uniform数据
        StructBufferWriter<PerFrameData> data(per_frame_uniform, BufferMapOption::WRITE_DISCARD);
        // 透视投影矩阵
        data->view_perspective_matrix = view_perspective.transpose();
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

    // ------------------------------------------------------
    const std::array<Plane, 6> worldspace_frustum = create_frustum_planes(view_perspective);

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
                if (m->submeshes[i].index_count == 0) {
                    continue;
                }
                if (!intersect_frustum_aabb(worldspace_frustum, mesh->aabbs[i])) {
                    continue; // 不在视椎体内部
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