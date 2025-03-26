#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/RenderTarget.h"

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
    }

    // 相机不受缩放属性影响，由组件更新
    Transform transform;

    float fov;
    float near_z, far_z;

    Viewport view_port; // 需要手动设置
    intrusive_ptr<RenderTarget> render_target; // 相机绘制的目标

    Matrix4 get_view_matrix() const noexcept {
        return Matrix4::rotate(transform.rotation).transpose() * Matrix4::translate(-transform.position);
    }
    Matrix4 get_perspective_matrix() const noexcept {
        float aspect = float(view_port.width) / float(view_port.height);
        return compute_perspective_matrix(aspect, fov, near_z, far_z);
    }
    Matrix4 get_view_perspective_matrix() const noexcept {
        return get_perspective_matrix() * get_view_matrix();
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
    Matrix4 normal_matrix;
    intrusive_ptr<Mesh> mesh;

    std::vector<intrusive_ptr<Material>> materials;
};

} // namespace Graphics
} // namespace Goonya