#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/graphics.h"

#include <vector>

namespace Goonya {
namespace Graphics {

struct CameraRenderInfo {
public:
    CameraRenderInfo() {
        near_z = 1.0f;
        far_z = 1000.0f;
        fov = 1.57f;
        view_port = {0, 0, 0, 0};
        view_matrix = Matrix4::identity();
    }

    // 由组件更新，不受缩放属性影响
    Matrix4 view_matrix;

    float fov;
    float near_z, far_z;

    Viewport view_port; // 需要手动设置
    intrusive_ptr<RenderTarget> render_target; // 相机绘制的目标

    Matrix4 get_view_matrix() const noexcept {
        // 进行一个与相机Transform相反的变换，无视scale
        return view_matrix;
    }
    Matrix4 get_perspective_matrix() const noexcept {
        float aspect = float(view_port.width) / float(view_port.height);
        return graphics_api->compute_perspective_matrix(aspect, fov, near_z, far_z, !render_target->is_screen());
    }
    Matrix4 get_view_perspective_matrix() const noexcept {
        // 先转换到相机坐标系，再投影
        return get_perspective_matrix() * get_view_matrix();
    }

    Matrix4 get_skybox_view_perspective_matrix() const noexcept {
        Matrix4 view = get_view_matrix();
        view.m[0][3] = 0;
        view.m[1][3] = 0;
        view.m[2][3] = 0;
        return get_perspective_matrix() * view;
    }
};

struct PointLight {
    Vector3f position;
    Vector3f color;
    float factor;
};

struct DirectionalLight {
    Vector3f direction;
    Vector3f flux;
};

struct Skybox {
    intrusive_ptr<Material> material;
    bool ignore_range;
    BoundingBox bbox;
};

struct MeshRenderInfo {
    Matrix4 model_matrix;
    Matrix3 normal_matrix;
    intrusive_ptr<Mesh> mesh;

    std::vector<intrusive_ptr<Material>> materials;
};

} // namespace Graphics
} // namespace Goonya