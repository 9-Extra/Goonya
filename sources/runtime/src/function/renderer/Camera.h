#pragma once

#include "core/RefCount.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/transform.h"
#include "core/cgmath/vector.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/opengl/GLRenderTarget.h"

namespace Goonya {

enum class CameraType {
    Perspective,
    Orthographic,
};

class RCamera {
public:
    Transform transform;

    CameraType type = CameraType::Perspective;
    float fov = 90.0f;
    float scale = 100.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    Vector4f rect = Vector4f(0.0f, 0.0f, 1.0f, 1.0f); // 屏幕上的矩形区域，默认全屏，最后两个参数是宽和高

    Ref<RenderTarget> render_target;

public:
    Matrix4f get_projection_matrix(float aspect_ratio) const noexcept {
        if (type == CameraType::Perspective) {
            return GL.compute_perspective_matrix(aspect_ratio, fov, near_plane, far_plane);
        } else {
            return GL.compute_orthographic_matrix(aspect_ratio, scale, near_plane, far_plane);
        }
    }

    Matrix4f get_view_matrix() const noexcept {
        return Matrix4f::identity().translate(-transform.position).rotate(transform.rotation.conjugate());
    }
    Matrix4f get_view_matrix_inversed() const noexcept {
        return Matrix4f::identity().rotate(transform.rotation).translate(transform.position);
    }

    Matrix4f get_skybox_view_matrix() const noexcept {
        return Matrix4f::identity().rotate(transform.rotation.conjugate());
    }
};

} // namespace Goonya