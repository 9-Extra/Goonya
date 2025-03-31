#pragma once

#include "core/cgmath.h"
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
            Matrix4 view_matrix = Matrix4::identity();
            for (GObject* parent = get_owner(); parent->has_parent(); parent = parent->get_parent().lock().get()) {
                const Transform& t = parent->get_transform();
                // 先将相机平移到原点，然后旋转到-z方向
                Matrix4 parent_view_matrix = Matrix4{t.rotation.conjugate().to_matrix()} * Matrix4::translate(-t.position);
                view_matrix = view_matrix * parent_view_matrix;
            }
            camera.view_matrix = view_matrix;
        }
    }
protected:
    CameraRenderInfo camera;
};
}
}
