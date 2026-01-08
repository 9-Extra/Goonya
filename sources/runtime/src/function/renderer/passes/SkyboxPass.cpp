#include "SkyboxPass.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/passes/UniformBufferStructure.h"

namespace Goonya {

// 渲染天空盒
void SkyBoxPass::run(PassRenderInfo &info) {
    if (info.skybox_material == nullptr) {
        return;
    }

    Matrix4f skybox_view_perspective_matrix =
        info.camera->get_skybox_view_perspective_matrix(info.screen_size[0] / info.screen_size[1]);
    Ref<GLBuffer> skybox_uniform = create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerFrameData));
    {
        // 填充天空盒需要的参数（透视投影矩阵）
        StructBufferAccessor<PerFrameData> data(skybox_uniform, BufferMapOption::WRITE_DISCARD);
        data->view_perspective_matrix = skybox_view_perspective_matrix.transpose();
        // 不需要normal_matrix
    }

    // 绑定天空盒材质
    info.skybox_material->bind();
    info.skybox_material->set_texture("skybox_specular_texture", info.env_map);
    skybox_uniform->bind_uniform(PER_FRAME_UNIFORM_BINDING);
    mesh->bind();
    GL.draw_submesh(mesh->submeshes.at(0));
}

} // namespace Goonya
