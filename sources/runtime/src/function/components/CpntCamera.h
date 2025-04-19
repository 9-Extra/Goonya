#pragma once

#include "core/cgmath.h"
#include "core/world/GObject.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/Renderer.h"
#include "platform/graphics/Graphics.h"

namespace Goonya::Graphics {
// 相机组件，可以设置为主相机，使相机跟随其owner object移动
class CpntCamera : public Component {
public:
    explicit CpntCamera(bool bind_to_screen = false, float near_z = 1.0f, float far_z = 1000.0f, float fov = 1.57) {
        camera_proxy.near_z = near_z;
        camera_proxy.far_z = far_z;
        camera_proxy.fov = fov;

        if (bind_to_screen) {
            camera_proxy.render_target = graphics_api->get_rendertarget_screen();
        }
    }

    bool should_be_main = false;

    void on_register() override { renderer.cameras.emplace(&camera_proxy); }

    void on_unregister() override { renderer.cameras.erase(&camera_proxy); }

    void on_tick() override {
        assert(get_owner() != nullptr);
        GObject &owner = *get_owner();
        if (owner.get_dirty_flag()[GObject::DirtyFlag::TRANSFORM_DIRTY]) {
            Matrix4 view_matrix = Matrix4::identity();
            for (GObject *parent = get_owner(); parent->has_parent(); parent = parent->get_parent().lock().get()) {
                const Transform &t = parent->get_transform();
                // 先将相机平移到原点，然后旋转到-z方向
                Matrix4 parent_view_matrix = Matrix4::translate(-t.position) * Matrix4::rotate(t.rotation.conjugate());
                view_matrix = parent_view_matrix * view_matrix;
            }
            camera_proxy.view_matrix = view_matrix;
            camera_proxy.camera_pos = get_owner()->get_world_model_matrix().resolve_translate();
        }
    }

protected:
    CameraRenderProxy camera_proxy;
};
} // namespace Goonya::Graphics
