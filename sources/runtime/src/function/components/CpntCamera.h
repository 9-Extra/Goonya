#pragma once

#include "function/renderer/Renderer.h"
#include "core/world/GObject.h"


namespace Goonya {
namespace Graphics {
// 相机组件，可以设置为主相机，使相机跟随其owner object移动
class CpntCamera: public Component{
public:
    CpntCamera(float near_z = 1.0f, float far_z = 1000.0f, float fov = 1.57): near_z(near_z), far_z(far_z), fov(fov), should_be_main(false) {}
    float near_z, far_z, fov;
    bool should_be_main;

    bool is_main_camera(){
        return renderer.active_camera != nullptr && renderer.active_camera == get_owner();
    }

    virtual void on_register() override{
        if (should_be_main){
            assert(get_owner() != nullptr);
            GObject& owner = *get_owner();
            renderer.active_camera = &owner;
        }
    }

    virtual void on_unregister() override{
        if (is_main_camera()){
            renderer.active_camera = nullptr;
        }
    }

    virtual void on_tick() override {
        assert(get_owner() != nullptr);
        GObject& owner = *get_owner();
        if (owner.get_dirty_flag()[GObject::DirtyFlag::TRANSFORM_DIRTY] && is_main_camera()){
            renderer.main_camera.far_z = far_z;
            renderer.main_camera.near_z = near_z;
            renderer.main_camera.fov = fov;

            renderer.main_camera.position = owner.get_transform().position;
            renderer.main_camera.rotation = owner.get_transform().rotation;
        }
    }
};
}
}
