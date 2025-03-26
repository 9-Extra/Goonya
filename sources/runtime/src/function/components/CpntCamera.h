#pragma once

#include "function/renderer/RenderAspect.h"
#include "function/renderer/Renderer.h"
#include "core/world/GObject.h"
#include "platform/graphics/graphics.h"


namespace Goonya {
namespace Graphics {
// 相机组件，可以设置为主相机，使相机跟随其owner object移动
class CpntCamera: public Component{
public:
    CpntCamera(bool bind_to_screen = false, float near_z = 1.0f, float far_z = 1000.0f, float fov = 1.57) {
        camera.near_z = near_z;
        camera.far_z = far_z;
        camera.fov = fov;    
        
        if (bind_to_screen){
            camera.render_target = graphics_api->get_rendertarget_screen();
        }
    }

    bool should_be_main;

    virtual void on_register() override{
        renderer.cameras.emplace(&camera);
    }

    virtual void on_unregister() override{
        renderer.cameras.erase(&camera);
    }

    virtual void on_tick() override {
        assert(get_owner() != nullptr);
        GObject& owner = *get_owner();
        if (owner.get_dirty_flag()[GObject::DirtyFlag::TRANSFORM_DIRTY]){
            camera.transform = owner.get_transform();
        }
    }
protected:
    CameraRenderInfo camera;
};
}
}
