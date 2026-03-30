#pragma once

#include "core/cgmath/transform.h"
#include "function/renderer/Camera.h"
#include "function/renderer/RScene.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "function/world/World.h"
#include "platform/graphics/opengl/GLRenderTarget.h"

namespace Goonya {
// 相机组件，可以设置为主相机，使相机跟随其owner object移动
class CpntCamera final : public Component {
public:
    float near_z;
    float far_z;
    float fov;
    Ref<RenderTarget> render_target;

    RScene *scene = nullptr;
    RCamera *p_camera = nullptr;

public:
    explicit CpntCamera(float near_z = 1.0f, float far_z = 1000.0f, float fov = 1.57)
        : near_z(near_z), far_z(far_z), fov(fov) {}

    std::unique_ptr<Component> clone() const override {
        auto new_comp = std::make_unique<CpntCamera>(near_z, far_z, fov);
        new_comp->render_target = render_target;
        return new_comp;
    }

    void on_register() override {
        scene = get_owner()->get_world()->get_scene();
        p_camera = scene->add_camera();
        p_camera->near_plane = near_z;
        p_camera->far_plane = far_z;
        p_camera->fov = fov;
        p_camera->render_target = render_target;
        p_camera->transform = Transform::from_matrix(get_owner()->get_world_model_matrix());
    }

    void on_unregister() override { scene->drop_camera(p_camera); }

    void on_update(ComponentUpdateFlag flag) override {
        this->Component::on_update(flag);
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            p_camera->transform = Transform::from_matrix(get_owner()->get_world_model_matrix());
        }
    }
};
} // namespace Goonya
