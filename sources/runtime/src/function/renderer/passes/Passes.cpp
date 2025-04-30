#include "Passes.h"

#include "../Renderer.h"
#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include "core/timer/timer.h"
#include "function/renderer/RenderAspect.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "resource/Resource.h"
#include <FreeImage.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Goonya::Graphics {

LambertianPass::LambertianPass() {
    per_frame_uniform = graphics_api->create_buffer(sizeof(PerFrameData), BufferType::DYNAMIC);
    per_frame_uniform->set_debug_label("Lambert Per Frame");
    per_object_uniform = graphics_api->create_buffer(sizeof(PerObjectData), BufferType::STREAM);
    per_object_uniform->set_debug_label("Lambert Per Object");
}
// 一般物体渲染
void LambertianPass::run() {
    const CameraRenderProxy *camera = renderer.current_camera;
    Vector3f camera_pos = camera->get_position();

    // 绑定per_frame和per_object uniform buffer
    per_frame_uniform->bind_uniform(0);
    per_object_uniform->bind_uniform(1);

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

    intrusive_ptr<Material> default_material = Resource::resources.materials.get("default");

    // 遍历所有part，绘制每一个part
    for (const MeshRenderProxy *mesh : renderer.meshes) {
        mesh->mesh->bind();
        
        // 填充PerObject参数
        {
            StructBufferWriter<PerObjectData> writer(per_object_uniform, BufferMapOption::WRITE_DISCARD);
            writer->model_matrix = mesh->model_matrix;
            writer->normal_matrix = mesh->normal_matrix;
        }
        const std::vector<SubMesh> submeshes = mesh->mesh->submeshes;
        // 逐一绘制子网格
        for (uint32_t i = 0; i < submeshes.size(); i++) {
            // 材质未设置时使用默认材质，多出来则无视
            intrusive_ptr<Material> current_material =
                mesh->materials.size() > i ? mesh->materials[i] : default_material;
            current_material->bind(); // 绑定材质
            graphics_api->draw_submesh(submeshes[i]);
        }
    }
}

// 渲染天空盒
void SkyBoxPass::run() {
    const CameraRenderProxy *camera = renderer.current_camera;
    Vector3f camera_pos = camera->get_position();

    // 寻找包含且最小，接近中心的天空盒
    Material *skybox_material = nullptr;
    float min_distance = std::numeric_limits<float>::infinity();
    for (Skybox &s : renderer.current_skyboxs) {
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

    Matrix4 skybox_view_perspective_matrix = camera->get_skybox_view_perspective_matrix();

    // 绑定天空盒材质
    skybox_material->bind();
    {
        // 填充天空盒需要的参数（透视投影矩阵）
        StructBufferWriter<SkyBoxData> data(skybox_uniform, BufferMapOption::WRITE_DISCARD);
        data->skybox_view_perspective_matrix = skybox_view_perspective_matrix.transpose();
    }
    skybox_uniform->bind_uniform(0);
    mesh->bind();
    graphics_api->draw_submesh(mesh->submeshes.at(0));
}

} // namespace Goonya::Graphics
