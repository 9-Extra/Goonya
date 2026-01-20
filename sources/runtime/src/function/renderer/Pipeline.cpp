#include "Pipeline.h"
#include "core/cgmath/transform.h"
#include "core/clock/GameClock.h"
#include "function/renderer/RenderScene.h"
#include "function/renderer/Renderer.h"
#include "platform/graphics/UberShader.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/ResMng.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"
#include <concepts>
#include <tuple>

namespace Goonya {

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

Pipeline::Pipeline() {
    skybox_mesh = resources.load_resource<GLMesh>("buildin:skybox_cube");
    if (!skybox_mesh) {
        throw RuntimeError("无法加载天空盒模型");
    }
    Ref<UberShader> postprocess_shader = resources.load_resource<UberShader>("shaders/post_process/basic");
    if (!postprocess_shader) {
        throw RuntimeError("无法加载后处理着色器");
    }
    postprocess_material = create_ref<Material>(postprocess_shader.get());

    VertexLayout layout = VertexLayoutBuilder().build();
    postprocess_quad_mesh = create_ref<GLMesh>(layout);
    // postprocess_quad_mesh->set_vertices(0, {});
    // postprocess_quad_mesh->set_indices({});
    postprocess_quad_mesh->submeshes.emplace_back(SubMesh{
        .start_index = 0,
        .index_count = 4,
        .base_vertex_offset = 0,
        .topology = Topology::TRIANGLE,
    });
}

void Pipeline::render() {
    auto [w, h] = GL.get_rendertarget_screen()->get_size();

    // 建立与屏幕大小相同的渲染目标
    for (auto &&rt : replace_render_target) {
        if (!rt || rt->get_size() != std::make_tuple(w, h)) {
            rt = create_ref<GLFrameBuffer>(std::make_tuple(w, h));
            rt->set_depth_stencil_renderbuffer(DepthStencilPixelFormat::DEPTH24_STENCIL8);
            Ref<GLTexture> color_texture =
                create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f8, std::make_tuple(w, h, 0));
            rt->attach_color_texture(0, color_texture);
            GN_ASSERT(rt->check_status());
        }
    }

    bool is_screen_painted = false;
    for (auto &&camera : renderer.camera_set) {
        if (!camera->render_target) continue;
        RenderScene *scene = renderer.scene_set.get_or_null(camera->scene);
        if (!scene) continue;

        if (camera->render_target->is_screen()) {
            is_screen_painted = true;
        }

        RenderContext context{.render_target = camera->render_target,
                              .camera = camera.get(),
                              .scene = scene,
                              .aspect_ratio = (float)w / (float)h};

        render_camera(context);
    }

    if (is_screen_painted) {
        // 将临时渲染目标的内容绘制到屏幕，因为在绘制时Y轴是倒的，所以这里需要翻转Y轴
        replace_render_target[0]->blit(GL.get_rendertarget_screen(), 0, 0, w, h, 0, h, w, 0, true, true, true);
    } else {
        LOG_ERROR("没有相机绑定到屏幕！");
    }
}
void Pipeline::render_camera(RenderContext &context) {
    CameraRenderProxy *camera = context.camera;
    auto [w, h] = camera->render_target->get_size();

    const Viewport viewport{(int32_t)(camera->rect.x * w), (int32_t)(camera->rect.y * h), (int32_t)(camera->rect.z * w),
                            (int32_t)(camera->rect.w * h)};
    GL.set_viewport(viewport);

    replace_render_target[0]->bind_draw();

    // 清除旧画面，todo: 根据相机的清除参数来清除
    GL.set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
    GL.clear(true, true, true);

    // 寻找包含且最小，接近中心的天空盒
    Skybox *skybox = nullptr;
    float min_distance = std::numeric_limits<float>::infinity();
    for (auto &&s : context.scene->skyboxs) {
        if (!s.ignore_range && !s.bbox.contains(camera->get_position())) {
            continue;
        }
        float d =
            s.ignore_range ? std::numeric_limits<float>::max() : (s.bbox.center() - camera->get_position()).square();
        if (d < min_distance) {
            skybox = &s;
            min_distance = d;
        }
    }

    context.env_map = skybox->env_map;
    context.skybox_material = skybox->skybox_material;

    Ref<GLBuffer> per_frame_uniform =
        create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerFrameData)); // 用于一般渲染每帧变化的数据
    Vector3f camera_pos = camera->get_position();
    RenderScene &scene = renderer.scene_set[camera->scene];

    const Matrix4f view_perspective = camera->get_view_projection_matrix(float(w) / float(h));

    // 绑定per_frame uniform buffer
    per_frame_uniform->bind_uniform(PER_FRAME_UNIFORM_BINDING);
    {
        // 填充per_frame uniform数据
        StructBufferAccessor<PerFrameData> data(per_frame_uniform, BufferMapOption::WRITE_DISCARD);
        // 透视投影矩阵
        data->view_perspective_matrix = view_perspective.transpose();
        // 视图矩阵
        data->view_matrix = camera->get_view_matrix().transpose();
        // 视图矩阵的逆矩阵
        data->view_matrix_inv = camera->get_view_matrix().inverse()->transpose();
        // 相机位置
        data->camera_position = camera_pos;
        // 雾参数
        GN_ASSERT(scene.fog_density >= 0.0f);
        data->fog_density = scene.fog_density;
        data->fog_min_distance = scene.fog_min_distance;
        data->time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(GAME_CLOCK.total()).count();
        data->screen_size = {(float)viewport.width, (float)viewport.height};
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
    draw_geometry(context);

    if (skybox) {
        draw_skybox(context);
    }

    draw_postprocess(context);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Pipeline::draw_geometry(RenderContext &context) {

    // ------------------------------------------------------
    const std::array<Plane, 6> worldspace_frustum =
        create_frustum_planes(context.camera->get_view_projection_matrix(context.aspect_ratio));

    Ref<Material> default_material = resources.load_resource<Material>("materials/default");

    struct Batch {
        const GLMesh *mesh;
        SubMesh sub_mesh;
        size_t per_object_data_offset;
    };

    std::unordered_map<Material *, std::vector<Batch>> batcher;

    // 把所有用于一般渲染每帧变化的数据收集到一个buffer中
    Ref<GLBuffer> per_object_uniform =
        create_ref<GLBuffer>(BufferType::MODIFIABLE, context.scene->mesh_proxys.size() * sizeof(PerObjectData));
    per_object_uniform->set_debug_label("Lambert Per Object");

    {
        ArrayBufferWriter<PerObjectData> per_object_data(per_object_uniform, BufferMapOption::WRITE_DISCARD);

        // 遍历所有part，绘制每一个part
        for (const auto [offset, mesh] : std::views::enumerate(context.scene->mesh_proxys)) {
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
        material->set_texture("skybox_specular_texture", context.env_map);

        for (Batch &item : batch) {
            per_object_uniform->bind_uniform_ranged(PER_OBJECT_UNIFORM_BINDING, item.per_object_data_offset,
                                                    sizeof(PerObjectData));
            item.mesh->bind();
            GL.draw_submesh(item.sub_mesh);
        }
    }
}

void Pipeline::draw_skybox(RenderContext &context) {
    if (context.skybox_material == nullptr) {
        return;
    }

    Matrix4f skybox_view_perspective_matrix = context.camera->get_skybox_view_perspective_matrix(context.aspect_ratio);
    Ref<GLBuffer> skybox_uniform = create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerFrameData));
    {
        // 填充天空盒需要的参数（透视投影矩阵）
        StructBufferAccessor<PerFrameData> data(skybox_uniform, BufferMapOption::WRITE_DISCARD);
        data->view_perspective_matrix = skybox_view_perspective_matrix.transpose();
        // 不需要normal_matrix
    }

    // 绑定天空盒材质
    context.skybox_material->bind();
    context.skybox_material->set_texture("skybox_specular_texture", context.env_map);
    skybox_uniform->bind_uniform(PER_FRAME_UNIFORM_BINDING);
    skybox_mesh->bind();
    GL.draw_submesh(skybox_mesh->submeshes.at(0));
}

void Pipeline::draw_postprocess(RenderContext &context) {
    // 后处理
    if (!postprocess_material) {
        return;
    }

    replace_render_target[1]->bind_draw();
    GL.clear(true, true, true);

    // replace_render_target[0]->blit(replace_render_target[1]);

    Ref<GLTexture> source_texture = replace_render_target[0]->get_color_texture(0);
    postprocess_material->set_texture("source", source_texture);
    postprocess_material->bind();
    GN_ASSERT_MSG(source_texture, "未获取到源纹理");
    postprocess_quad_mesh->bind();
    GL.draw_vertices(Topology::TRIANGLE, 0, 6);

    std::ranges::swap(replace_render_target[0], replace_render_target[1]);
}

} // namespace Goonya