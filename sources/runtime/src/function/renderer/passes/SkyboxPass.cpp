#include "SkyboxPass.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/Renderer.h"

namespace Goonya::Graphics {

// 渲染天空盒
void SkyBoxPass::run(CameraRenderProxy *camera) {
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