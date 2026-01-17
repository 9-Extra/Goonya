#pragma once

#include "core/cgmath/cgmath.h"
#include "core/sparse_set.h"
#include "function/renderer/RenderScene.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/opengl/OpenGLAPI.h"

namespace Goonya {

class CameraRenderProxy {
public:
    // 由组件更新，不受缩放属性影响
    Matrix4f view_matrix = Matrix4f::identity();
    Vector3f camera_pos{0, 0, 0};

    float fov = 1.57f;
    float near_z = 1000.0f, far_z = 1.0f;

    Vector4f rect = {0, 0, 1, 1};    // 按照比例计算，其实是[x, y, w, h]
    Ref<RenderTarget> render_target; // 相机绘制的目标
    Handle<RenderScene> scene;       // 绘制的场景

public:
    CameraRenderProxy() = default;

    Vector3f get_position() const noexcept { return camera_pos; }

    Matrix4f get_view_matrix() const noexcept { return view_matrix; }
    Matrix4f get_projection_matrix(float ratio) const noexcept {
        return GL.compute_perspective_matrix(ratio, fov, near_z, far_z);
    }
    Matrix4f get_view_projection_matrix(float ratio) const noexcept {
        // 先转换到相机坐标系，再投影
        return get_view_matrix() * get_projection_matrix(ratio);
    }

    Matrix4f get_skybox_view_perspective_matrix(float ratio) const noexcept {
        // 用于天空盒的透视投影矩阵（移除位移）
        Matrix4f view = get_view_matrix();
        view[3, 0] = 0;
        view[3, 1] = 0;
        view[3, 2] = 0;
        return view * get_projection_matrix(ratio);
    }
};

} // namespace Goonya
