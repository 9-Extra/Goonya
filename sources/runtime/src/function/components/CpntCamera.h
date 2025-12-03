#pragma once

#include "core/cgmath/cgmath.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/Renderer.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "function/world/World.h"
#include "platform/graphics/RenderTarget.h"

namespace Goonya::Graphics {
// 相机组件，可以设置为主相机，使相机跟随其owner object移动
class CpntCamera final : public Component {
public:
    Ref<RenderTarget> render_target;

protected:
    CameraRenderProxy *camera_proxy = nullptr;
    TickFunction* ticker = nullptr;

private:
    float near_z;
    float far_z;
    float fov;

public:
    explicit CpntCamera(float near_z = 1.0f, float far_z = 1000.0f, float fov = 1.57)
        : near_z(near_z), far_z(far_z), fov(fov) {}

    void on_register() override {
        camera_proxy = renderer.create_camera();

        camera_proxy->near_z = near_z;
        camera_proxy->far_z = far_z;
        camera_proxy->fov = fov;
        camera_proxy->render_target = render_target;
        camera_proxy->scene = &get_owner()->get_world()->main_scene();
    }

    void on_unregister() override { renderer.drop_camera(camera_proxy); }

    void on_update(ComponentUpdateFlag flag) override {
        this->Component::on_update(flag);
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            Matrix4 view_matrix = Matrix4::identity();
            for (GObject *parent = get_owner(); parent->has_parent(); parent = parent->get_parent().lock().get()) {
                const Transform &t = parent->get_local_transform();
                // 先将相机平移到原点，然后旋转到-z方向
                Matrix4 parent_view_matrix = Matrix4::identity().translate(-t.position).rotate(t.rotation.conjugate());
                view_matrix = parent_view_matrix * view_matrix;
            }
            camera_proxy->view_matrix = view_matrix;
            camera_proxy->camera_pos = get_owner()->get_world_model_matrix().resolve_position();
        }
    }
};
} // namespace Goonya::Graphics
