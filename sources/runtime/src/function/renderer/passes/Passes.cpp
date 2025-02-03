#include "Passes.h"

#include "../Renderer.h"
#include "core/intrusive_ptr.h"
#include "function/renderer/RenderItem.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/GraphicsResource.h"
#include "platform/graphics/graphics.h"

namespace Goonya {
namespace Graphics {

// 一般物体渲染
void LambertianPass::run() {
    // 初始化渲染配置
    graphics_api->bind_rendertarget_screen();
    Renderer::Viewport &v = renderer.main_viewport;
    graphics_api->set_viewport(v.x, v.y, v.width, v.height);
    // 清除旧画面
    graphics_api->set_clear_parameter(Color{0.0f, 0.0f, 0.0f});
    graphics_api->clear();
    checkError();
    // 绑定per_frame和per_object uniform buffer
    per_frame_uniform->bind_uniform(0);
    per_object_uniform->bind_uniform(1);
    checkError();
    // 计算透视投影矩阵
    const float aspect = float(v.width) / float(v.height);
    const Camera &camera = renderer.main_camera;
    const Matrix4 view_perspective_matrix = compute_perspective_matrix(aspect, camera.fov, camera.near_z, camera.far_z) *
                                           Matrix4::rotate(camera.rotation).transpose() *
                                           Matrix4::translate(-camera.position);
    checkError();
    {
        // 填充per_frame uniform数据
        StructBufferWriter<PerFrameData> data(per_frame_uniform);
        // 透视投影矩阵
        data->view_perspective_matrix = view_perspective_matrix.transpose();
        // 相机位置
        data->camera_position = camera.position;
        // 雾参数
        assert(renderer.fog_density >= 0.0f);
        data->fog_density = renderer.fog_density;
        data->fog_min_distance = renderer.fog_min_distance;
        // 灯光参数
        data->ambient_light = renderer.ambient_light;
        if (renderer.pointlights.size() > POINTLIGNT_MAX) {
            std::cout << "超出最大点光源数量" << std::endl;
        }
        uint32_t count = (uint32_t)std::min<size_t>(renderer.pointlights.size(), POINTLIGNT_MAX);
        for (uint32_t i = 0; i < count; ++i) {
            data->pointlight_list[i].position = renderer.pointlights[i].position;
            data->pointlight_list[i].intensity = renderer.pointlights[i].color * renderer.pointlights[i].factor;
        }
        data->pointlight_num = count;
        // 填充结束
    }
    checkError();
    // 遍历所有part，绘制每一个part
    for (const RenderItem *p : parts) {
        // 查找并绑定材质
        p->material->bind();
        {
            // 填充per_object uniform buffer
            StructBufferWriter<PerObjectData> data(per_object_uniform);
            data->model_matrix = p->world_model_matrix.transpose();      // 变换矩阵
            data->normal_matrix = p->world_normal_matrix.transpose(); // 法线变换矩阵
        }
        graphics_api->draw(p->mesh);
    }
}

// 渲染天空盒
void SkyBoxPass::run() {
    const Camera &camera = renderer.main_camera;
    Vector3f camera_pos = camera.position;

    // 寻找包含且最小，接近中心的天空盒
    Material* skybox_material = nullptr;
    float min_distance = std::numeric_limits<float>::infinity();
    for (const Skybox &s : renderer.current_skyboxs) {
        if (!s.ignore_range && !s.bbox.contains(camera_pos)) {
            continue;
        }
        float d = s.ignore_range ? std::numeric_limits<float>::max() : (s.bbox.center() - camera_pos).square();
        if (d < min_distance) {
            skybox_material = s.material.get();
            min_distance = d;
        }
    }

    if (skybox_material == nullptr) {
        return; // 没有合适的天空盒，跳过
    }

    graphics_api->bind_rendertarget_screen();

    // 用于天空盒的投影矩阵
    const float aspect = float(renderer.main_viewport.width) / float(renderer.main_viewport.height);
    const Matrix4 skybox_view_perspective_matrix =
        compute_perspective_matrix(aspect, camera.fov, camera.near_z, camera.far_z) *
        Matrix4::rotate(camera.rotation).transpose();

    // 绑定天空盒材质
    skybox_material->bind();
    {
        // 填充天空盒需要的参数（透视投影矩阵）
        StructBufferWriter<SkyBoxData> data(skybox_uniform);
        data->skybox_view_perspective_matrix = skybox_view_perspective_matrix.transpose();
    }
    skybox_uniform->bind_uniform(0);
    graphics_api->draw(mesh);
}

} // namespace Graphics
} // namespace Goonya