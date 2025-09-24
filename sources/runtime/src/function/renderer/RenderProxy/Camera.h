#pragma once

#include "core/cgmath.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/RenderTarget.h"

namespace Goonya::Graphics {

class CameraRenderProxy {
public:
    // 由组件更新，不受缩放属性影响
    Matrix4 view_matrix;
    Vector3f camera_pos;

    float fov;
    float near_z, far_z;
    
    Viewport view_port = {0, 0, 0, 0};         // 需要手动设置
    Ref<RenderTarget> render_target; // 相机绘制的目标

public:
    CameraRenderProxy() {
        near_z = 1.0f;
        far_z = 1000.0f;
        fov = 1.57f;
        view_matrix = Matrix4::identity();
    }

    Vector3f get_position() const noexcept { return camera_pos; }

    Matrix4 get_view_matrix() const noexcept { return view_matrix; }
    Matrix4 get_perspective_matrix() const noexcept {
        float aspect = static_cast<float>(view_port.width) / static_cast<float>(view_port.height);
        return graphics_api->compute_perspective_matrix(aspect, fov, near_z, far_z, !render_target->is_screen());
    }
    Matrix4 get_view_perspective_matrix() const noexcept {
        // 先转换到相机坐标系，再投影
        return get_view_matrix() * get_perspective_matrix();
    }

    Matrix4 get_skybox_view_perspective_matrix() const noexcept {
        // 用于天空盒的透视投影矩阵（移除位移）
        Matrix4 view = get_view_matrix();
        view[3, 0] = 0;
        view[3, 1] = 0;
        view[3, 2] = 0;
        return view * get_perspective_matrix();
    }
};

} // namespace Goonya::Graphics
