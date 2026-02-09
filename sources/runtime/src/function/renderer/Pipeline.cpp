#include "Pipeline.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/transform.h"
#include "core/clock/GameClock.h"
#include "function/renderer/Material.h"
#include "function/renderer/PipelineLayout.h"
#include "function/renderer/RScene.h"
#include "function/renderer/Renderer.h"
#include "function/renderer/UberShader.h"
#include "imgui.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLRenderTarget.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/ResMng.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
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
    skybox_material = create_ref<Material>(resources.load_resource<UberShader>("shaders/skybox/skybox"));

    if (!skybox_mesh) {
        throw RuntimeError("无法加载天空盒模型");
    }
    {
        depth_material = create_ref<Material>(resources.load_resource<UberShader>("shaders/depth/depth"));
        // 深度着色器只写入深度
        depth_material->set_pipeline_setting("_write_red", (PipelineSettingParamType) false);
        depth_material->set_pipeline_setting("_write_green", (PipelineSettingParamType) false);
        depth_material->set_pipeline_setting("_write_blue", (PipelineSettingParamType) false);
        depth_material->set_pipeline_setting("_write_alpha", (PipelineSettingParamType) false);
        depth_material->set_pipeline_setting("_write_stencil", (PipelineSettingParamType) false);
        depth_material->set_pipeline_setting("_write_depth", (PipelineSettingParamType) true);
        if (!depth_material) {
            throw RuntimeError("无法加载深度着色器");
        }
    }
    Ref<UberShader> postprocess_shader = resources.load_resource<UberShader>("shaders/post_process/basic");
    if (!postprocess_shader) {
        throw RuntimeError("无法加载后处理着色器");
    }
    postprocess_material = create_ref<Material>(postprocess_shader.get());

    postprocess_quad_mesh = create_ref<GLMesh>(VertexLayoutBuilder().build()); // 空的网格体

    guassian_blur_material_horizontal =
        create_ref<Material>(resources.load_resource<UberShader>("shaders/post_process/guass"));
    if (!guassian_blur_material_horizontal) {
        throw RuntimeError("无法加载高斯模糊水平着色器");
    }
    guassian_blur_material_horizontal->set_local_variant_key("HORIZONTAL");
    guassian_blur_material_vertical = guassian_blur_material_horizontal->clone();
    guassian_blur_material_vertical->set_local_variant_key("VERTICAL");
    if (!guassian_blur_material_vertical) {
        throw RuntimeError("无法加载高斯模糊垂直着色器");
    }
    bright_extract_material =
        create_ref<Material>(resources.load_resource<UberShader>("shaders/post_process/extract_bright"));
    if (!bright_extract_material) {
        throw RuntimeError("无法加载亮度提取着色器");
    }
}

void Pipeline::render() {
    ImGui::Begin("Rendering");

    auto [w, h] = GL.get_rendertarget_screen()->get_size();

    // 建立与屏幕大小相同的渲染目标
    for (auto &&rt : replace_render_target) {
        if (!rt || rt->get_size() != std::make_tuple(w, h)) {
            rt = create_ref<GLFrameBuffer>(std::make_tuple(w, h));
            rt->set_depth_stencil_renderbuffer(TextureStorageFormat::DEPTH_24_STENCIL_8);
            Ref<GLTexture> color_texture = create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f16,
                                                                 std::make_tuple(w, h, 0), 1);
            rt->attach_color_texture(0, color_texture);
            GN_ASSERT(rt->check_status());
        }
    }

    bool is_screen_painted = false;
    for (auto [i, scene] : std::views::enumerate(renderer.scenes)) {
        GN_ASSERT(scene);
        for (RCamera *camera : scene->cameras) {
            GN_ASSERT(camera);
            if (!camera->render_target) continue;

            if (camera->render_target->is_screen()) {
                is_screen_painted = true;
            }
            ImGui::SeparatorText(std::format("Camera {}", i).c_str());
            render_camera(camera, scene);
        }
    }

    if (is_screen_painted) {
        // 将临时渲染目标的内容绘制到屏幕，因为在绘制时Y轴是倒的，所以这里需要翻转Y轴
        replace_render_target[0]->blit(GL.get_rendertarget_screen(), 0, 0, w, h, 0, h, w, 0, true, false, false);
    } else {
        LOG_ERROR("没有相机绑定到屏幕！");
    }
    ImGui::End();
}
void Pipeline::render_camera(RCamera *camera, RScene *scene) {
    auto [w, h] = camera->render_target->get_size();

    const Viewport viewport{.x = (int32_t)(camera->rect.x * w),
                            .y = (int32_t)(camera->rect.y * h),
                            .width = (uint32_t)(camera->rect.z * w),
                            .height = (uint32_t)(camera->rect.w * h)};
    GL.set_viewport(viewport);

    replace_render_target[0]->bind_draw();

    // 清除旧画面，todo: 根据相机的清除参数来清除
    GL.set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
    GL.clear(true, true, true);

    // 寻找包含且最小，接近中心的天空盒
    REnvironment *env = nullptr;
    float min_distance = std::numeric_limits<float>::infinity();
    for (auto &&s : scene->environments) {
        if (!s.is_infinite && !s.aabb.contains(camera->transform.position)) {
            continue;
        }
        float d =
            s.is_infinite ? std::numeric_limits<float>::max() : (s.aabb.center() - camera->transform.position).square();
        if (d < min_distance) {
            env = &s;
            min_distance = d;
        }
    }

    const float ratio = float(viewport.width) / float(viewport.height);
    const Matrix4f view_matrix = camera->get_view_matrix();
    const Matrix4f projection_matrix = camera->get_projection_matrix(ratio);
    const Matrix4f view_projection_matrix = view_matrix * projection_matrix;
    Vector3f camera_pos = camera->transform.position;

    CameraInfo camera_info{
        .aspect_ratio = ratio,
        .env = env,
        .camera = camera,
        .scene = scene,
        .view_matrix = view_matrix,
        .projection_matrix = projection_matrix,
        .view_projection_matrix = view_projection_matrix,
    };

    Ref<GLBuffer> per_frame_uniform =
        create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerFrameData)); // 用于一般渲染每帧变化的数据

    {
        // 填充per_frame uniform数据
        StructBufferAccessor<PerFrameData> data(per_frame_uniform, BufferMapOption::WRITE_DISCARD);
        // 视图矩阵
        data->view_matrix = view_matrix.transpose();
        // 视图矩阵的逆矩阵
        data->view_matrix_inv = camera->get_view_matrix_inversed().transpose();
        data->perspective_matrix = projection_matrix.transpose();
        // 透视投影矩阵
        data->view_perspective_matrix = view_projection_matrix.transpose();
        // 相机位置
        data->camera_position = camera_pos;
        // 雾参数
        GN_ASSERT(env->fog_density >= 0.0f);
        data->fog_density = env->fog_density;
        data->fog_min_distance = env->fog_min_distance;
        data->time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(GAME_CLOCK.total()).count();
        data->screen_size = {(float)viewport.width, (float)viewport.height};
        // 灯光参数
        data->ambient_light = env->ambient_light;
        if (scene->lights.size() > POINTLIGHT_MAX) {
            LOG_WARN("点光源数量({})超出上限({})", scene->lights.size(), POINTLIGHT_MAX);
        }
        uint32_t count = static_cast<uint32_t>(std::min<size_t>(scene->lights.size(), POINTLIGHT_MAX));
        for (const auto &[i, l] : std::views::enumerate(scene->lights)) {
            data->pointlight_list[i].position = l.position;
            data->pointlight_list[i].intensity = l.linear_color * l.intensity;
        }
        data->pointlight_num = count;
        // 填充结束
    }
    // 绑定per_frame uniform buffer
    per_frame_uniform->bind_uniform(PER_FRAME_UNIFORM_BINDING);

    std::vector<CullInstance> visible_instances = cull(camera_info);

    draw_depth(camera_info, visible_instances);

    draw_geometry(camera_info, visible_instances);

    draw_skybox(camera_info);

    draw_postprocess(camera_info);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<CullInstance> Pipeline::cull(CameraInfo &camera_info) {

    std::vector<CullInstance> visible_instances;

    const std::array<Plane, 6> worldspace_frustum = create_frustum_planes(camera_info.view_projection_matrix);

    for (auto &&instance : camera_info.scene->instances) {
        if (!intersect_frustum_aabb(worldspace_frustum, instance.transformed_bbox)) {
            continue; // 不在视椎体内部
        }

        GN_ASSERT(instance.material);

        visible_instances.emplace_back(CullInstance{
            .mesh = instance.mesh.get(),
            .material = instance.material.get(),
            .submesh = instance.submesh,
            .per_object_uniform = instance.per_object_uniform.get(),
        });
    }
    return visible_instances;
}
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Pipeline::draw_depth(CameraInfo &camera_info, std::vector<CullInstance> &visible_instances) {
    GL.push_debug_group_label("Draw Depth");
    auto [w, h] = replace_render_target[0]->get_size();
    if (!depth_fbo || depth_fbo->get_size() != std::make_tuple(w, h)) {
        depth_fbo = create_ref<GLFrameBuffer>(std::make_tuple(w, h));
        depth_texture = create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::DEPTH_24_STENCIL_8,
                                              std::make_tuple(w, h, 0), 1);
        depth_fbo->set_depth_texture(depth_texture);
        GN_ASSERT(depth_fbo->check_status());
    }

    depth_fbo->bind_draw();
    GL.clear(false, true, false);

    depth_material->bind();

    for (auto &instance : visible_instances) {
        instance.mesh->bind();
        instance.per_object_uniform->bind_uniform(PER_OBJECT_UNIFORM_BINDING);
        GL.draw_submesh(instance.submesh);
    }
    GL.pop_debug_group_label();
}
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Pipeline::draw_geometry(CameraInfo &camera_info, std::vector<CullInstance> &visible_instances) {
    GL.push_debug_group_label("Draw Geometry");
    replace_render_target[0]->bind_draw();

    ImGui::BulletText("Visible Instances: %zu", visible_instances.size());
    for (auto &instance : visible_instances) {
        instance.material->set_texture("skybox_specular_texture", camera_info.env->environment_map);
        instance.material->set_texture("camera_depth", depth_texture);
        instance.material->bind();
        instance.mesh->bind();
        instance.per_object_uniform->bind_uniform(PER_OBJECT_UNIFORM_BINDING);
        GL.draw_submesh(instance.submesh);
    }
    GL.pop_debug_group_label();
}

void Pipeline::draw_skybox(CameraInfo &camera_info) {
    if (camera_info.env->skybox == nullptr) {
        return;
    }
    GL.push_debug_group_label("Draw Skybox");

    replace_render_target[0]->bind_draw();

    Matrix4f skybox_view_perspective_matrix =
        (camera_info.camera->get_skybox_view_matrix() * camera_info.projection_matrix).transpose();
    Ref<GLBuffer> skybox_per_pass =
        create_ref<GLBuffer>(BufferType::DEVICE_ONLY, std::as_bytes(std::span(&skybox_view_perspective_matrix, 1)));
    skybox_per_pass->bind_uniform(PER_PASS_UNIFORM_BINDING);
    // 绑定天空盒材质
    skybox_material->bind();
    skybox_material->set_texture("skybox_specular_texture", camera_info.env->skybox);
    skybox_material->set_param("color_permutation", camera_info.env->skybox_permutation);
    skybox_mesh->bind();
    GL.draw_submesh(skybox_mesh->submeshes.at(0));

    GL.pop_debug_group_label();
}

void Pipeline::draw_postprocess(CameraInfo &camera_info) {
    GL.push_debug_group_label("Draw Postprocess");
    // 后处理
    auto [width, height] = GL.get_rendertarget_screen()->get_size();
    width /= 2;
    height /= 2;

    if (renderer.draw_bloom) {
        if (!bloom_render_target[0] || bloom_render_target[0]->get_size() != std::make_tuple(width, height)) {
            for (auto &target : bloom_render_target) {
                target = create_ref<GLFrameBuffer>(std::make_tuple(width, height));
                Ref<GLTexture> color_texture = create_ref<GLTexture>(
                    TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f16, std::make_tuple(width, height, 0), 1);
                color_texture->set_warp_mode(TextureWarpMode::ClAMP);
                color_texture->set_filter_mode(TextureFilterMode::BILINEAR);
                target->attach_color_texture(0, color_texture);
            }
        }

        screen_paint(bloom_render_target[0], replace_render_target[0], bright_extract_material);
        screen_paint(bloom_render_target[1], bloom_render_target[0], guassian_blur_material_horizontal);
        screen_paint(bloom_render_target[0], bloom_render_target[1], guassian_blur_material_vertical);

        postprocess_material->set_local_variant_key("BLOOM");
        postprocess_material->set_texture("bloom", bloom_render_target[0]->get_color_texture(0));
        screen_paint(replace_render_target[1], replace_render_target[0], postprocess_material);

        // 保证最终结果在replace_render_target[0]
        std::ranges::swap(replace_render_target[0], replace_render_target[1]);
    } else {
        postprocess_material->remove_local_variant_key("BLOOM");
        screen_paint(replace_render_target[1], replace_render_target[0], postprocess_material);

        // 保证最终结果在replace_render_target[0]
        std::ranges::swap(replace_render_target[0], replace_render_target[1]);
    }
    GL.pop_debug_group_label();
}

void Pipeline::screen_paint(Ref<GLFrameBuffer> dst, Ref<GLFrameBuffer> src, Ref<Material> material) {
    dst->bind_draw();
    material->set_texture("source", src->get_color_texture(0));
    material->bind();
    postprocess_quad_mesh->bind();
    auto [w, h] = dst->get_size();
    GL.set_viewport(Viewport{0, 0, w, h});
    GL.draw_vertices(Topology::TRIANGLE, 0, 6);
}

} // namespace Goonya