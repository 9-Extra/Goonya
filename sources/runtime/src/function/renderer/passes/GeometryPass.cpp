#include "GeometryPass.h"
#include "core/RefCount.h"
#include "core/cgmath/cgmath.h"
#include "core/cgmath/transform.h"
#include "core/log/Log.h"
#include "function/renderer/RenderScene.h"
#include "function/renderer/Renderer.h"
#include "function/renderer/passes/UniformBufferStructure.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/ResMng.h"

#include <array>

#include <cstddef>
#include <cstdint>

namespace Goonya {

GeometryPass::GeometryPass() {
    per_frame_uniform = create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerFrameData));
    per_frame_uniform->set_debug_label("Lambert Per Frame");
}

/**
 * @brief 从矩阵重建视锥体
 *
 * @param mat 任意可以投影到OpenGL裁剪空间的矩阵（乘在右边的版本，Y轴翻转没有实际影响）
 * @return 视锥体的6个平面，法线向视椎体内，没有归一化
 */
std::array<Plane, 6> create_frustum_planes(const Matrix4f &mat) noexcept {
    Matrix4f col = mat.transpose();

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
void GeometryPass::run(PassRenderInfo &info) {
    Vector3f camera_pos = info.camera->get_position();
    RenderScene &scene = renderer.scene_set[info.camera->scene];

    const Matrix4f view_perspective =
        info.camera->get_view_projection_matrix(info.screen_size[0] / info.screen_size[1]);

    // 绑定per_frame uniform buffer
    per_frame_uniform->bind_uniform(PER_FRAME_UNIFORM_BINDING);
    {
        // 填充per_frame uniform数据
        StructBufferAccessor<PerFrameData> data(per_frame_uniform, BufferMapOption::WRITE_DISCARD);
        // 透视投影矩阵
        data->view_perspective_matrix = view_perspective.transpose();
        // 视图矩阵
        data->view_matrix = info.camera->get_view_matrix().transpose();
        // 视图矩阵的逆矩阵
        data->view_matrix_inv = info.camera->get_view_matrix().inverse()->transpose();
        // 相机位置
        data->camera_position = camera_pos;
        // 雾参数
        GN_ASSERT(scene.fog_density >= 0.0f);
        data->fog_density = scene.fog_density;
        data->fog_min_distance = scene.fog_min_distance;
        data->time = info.time;
        data->screen_size = info.screen_size;
        // 灯光参数
        data->ambient_light = scene.ambient_light;
        if (scene.pointlights.size() > POINTLIGHT_MAX) {
            LOG_WARN("点光源数量({})超出上限({})", scene.pointlights.size(), POINTLIGHT_MAX);
        }
        uint32_t count = static_cast<uint32_t>(std::min<size_t>(scene.pointlights.size(), POINTLIGHT_MAX));
        for (const auto &[i, l] : std::views::enumerate(scene.pointlights)) {
            data->pointlight_list[i].position = l.position;
            data->pointlight_list[i].intensity = l.color * l.factor;
        }
        data->pointlight_num = count;
        // 填充结束
    }

    // ------------------------------------------------------
    const std::array<Plane, 6> worldspace_frustum = create_frustum_planes(view_perspective);

    Ref<Material> default_material = resources.load_resource<Material>("materials/default");

    struct Batch {
        const GLMesh *mesh;
        SubMesh sub_mesh;
        size_t per_object_data_offset;
    };

    std::unordered_map<Material *, std::vector<Batch>> batcher;

    // 把所有用于一般渲染每帧变化的数据收集到一个buffer中
    Ref<GLBuffer> per_object_uniform =
        create_ref<GLBuffer>(BufferType::MODIFIABLE, scene.mesh_proxys.size() * sizeof(PerObjectData));
    per_object_uniform->set_debug_label("Lambert Per Object");

    {
        ArrayBufferWriter<PerObjectData> per_object_data(per_object_uniform, BufferMapOption::WRITE_DISCARD);

        // 遍历所有part，绘制每一个part
        for (const auto [offset, mesh] : std::views::enumerate(scene.mesh_proxys)) {
            const GLMesh *m = mesh->mesh.get();

            // 填充PerObject参数
            per_object_data[offset]->model_matrix = mesh->model_matrix.transpose();
            per_object_data[offset]->normal_matrix = Matrix4f{mesh->normal_matrix.transpose()};

            for (uint32_t i = 0; i < m->submeshes.size(); i++) {
                if (m->submeshes[i].index_count == 0) {
                    continue;
                }
                if (!intersect_frustum_aabb(worldspace_frustum, mesh->aabbs[i])) {
                    continue; // 不在视椎体内部
                }

                // 材质未设置时使用默认材质，多出来则无视
                bool has_material = i < mesh->materials.size() && bool(mesh->materials[i]);
                auto current_material = has_material ? mesh->materials[i] : default_material;

                Batch batch{m, m->submeshes[i], offset * sizeof(PerObjectData)};

                batcher[current_material.get()].emplace_back(batch);
            }
        }
    }

    for (auto &[material, batch] : batcher) {
        material->bind();
        material->set_texture("skybox_specular_texture", info.env_map);

        for (Batch &item : batch) {
            per_object_uniform->bind_uniform_ranged(PER_OBJECT_UNIFORM_BINDING, item.per_object_data_offset,
                                                    sizeof(PerObjectData));
            item.mesh->bind();
            GL.draw_submesh(item.sub_mesh);
        }
    }
}

} // namespace Goonya
