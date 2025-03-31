#include "logic.h"
#include "core/cgmath.h"
#include "core/input/input.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include <function/renderer/RenderResource.h>
#include <platform/graphics/Shader.h>

#include <FreeImage.h>

void MoveSystem::handle_mouse() {
    using namespace Goonya;
    if (Goonya::Input::is_mouse_click(Goonya::Input::LEFT)) {
        lights->enable();
    }
    if (Goonya::Input::is_mouse_click(Goonya::Input::RIGHT)) {
        lights->disable();
    }

    // 使用鼠标中键旋转视角
    //  左上角为(0,0)，右下角为(w,h)
    if (Goonya::Input::get_mouse_state(Goonya::Input::MIDDLE) == Input::KeyState::DOWN) {
        auto [dx, dy] = Goonya::Input::get_mouse_move();
        // 鼠标向右拖拽，相机沿全局坐标系y轴顺时针旋转。鼠标向下拖拽时，相机沿局部坐标系x轴顺时针旋转
        const float rotate_speed = 0.003f;
        camera->rotate_local_axis({dy * rotate_speed, 0, 0});
        camera->rotate_global_axis({0, dx * rotate_speed, 0});
    }
}
void MoveSystem::handle_keyboard(float delta) {
    // wasd移动
    using namespace Goonya;
    const float move_speed = 0.02f * delta;

    if (Goonya::Input::is_key_click('F')) {
        static const std::string key_name = "GYA_IBL_ENVIRONMENT_LIGHT";
        Graphics::ShaderLib *shader_lib = Graphics::resources.shader_lib.get();
        if (!shader_lib->is_global_varient_key_set(key_name)) {
            shader_lib->set_global_varient_key(key_name);
        } else {
            shader_lib->reset_global_varient_key(key_name);
        }
    }

    if (Goonya::Input::is_key_click('P')){
        LOG_DEBUG("正在进行图像导出");
        intrusive_ptr<Graphics::Texture> skybox = Graphics::resources.textures.at("skybox_valley_color");
        FIBITMAP* image = skybox->export_image(0);
        FreeImage_AdjustGamma(image, 2.2);
        if (!FreeImage_Save(FIF_BMP, image, "output.bmp")){
            LOG_ERROR("图像导出失败");
        };
        FreeImage_Unload(image);
    }

    if (Goonya::Input::is_key_click(Goonya::Input::KeyCode::ESCAPE)) {
        Goonya::EventBus::dispatch_event(Goonya::Events::EngineStop{});
    }
    const Goonya::Transform &trans = camera->get_transform();
    if (Goonya::Input::get_key_state('W') == Input::KeyState::DOWN) {
        Vector3f ori = trans.get_forward_direction();
        ori.y = 0.0;
        camera->translate(ori.normalize() * move_speed);
    }
    if (Goonya::Input::get_key_state('S') == Input::KeyState::DOWN) {
        Vector3f ori = trans.get_forward_direction();
        ori.y = 0.0;
        camera->translate(ori.normalize() * -move_speed);
    }
    if (Goonya::Input::get_key_state('A') == Input::KeyState::DOWN) {
        Vector3f ori = trans.get_forward_direction();
        ori = {ori.z, 0.0, -ori.x};
        camera->translate(ori.normalize() * move_speed);
    }
    if (Goonya::Input::get_key_state('D') == Input::KeyState::DOWN) {
        Vector3f ori = trans.get_forward_direction();
        ori = {ori.z, 0.0, -ori.x};
        camera->translate(ori.normalize() * -move_speed);
    }
    if (Goonya::Input::get_key_state(Goonya::Input::KeyCode::SPACE) == Input::KeyState::DOWN) {
        camera->translate({0.0f, move_speed, 0.0f});
    }
    if (Goonya::Input::get_key_state(Goonya::Input::KeyCode::LSHIFT) == Input::KeyState::DOWN) {
        camera->translate({0.0f, -move_speed, 0.0f});
    }

    if (Goonya::Input::is_key_click('0')) {
        camera->set_transform(Goonya::Transform{});
    }

    // Vector3f pos = camera->get_transform().position;
    // LOG_DEBUG("x = {}, y = {}, z = {}", pos.x, pos.y, pos.z);

    // if (Goonya::Input::is_key_down(VK_UP)) {
    //     renderer.fog_density += 0.00001f * delta;
    // }

    // if (Goonya::Input::is_key_down(VK_DOWN)) {
    //     renderer.fog_density -= 0.00001f * delta;
    //     renderer.fog_density = std::max(renderer.fog_density, 0.0f);
    // }
}
void MoveSystem::on_register() {
    cube = get_owner()->get_child_by_name("方块");
    assert(cube);
    lights = get_owner()->get_child_by_name("lights");
    assert(lights);
    light1 = get_owner()->get_child_by_name("lights")->get_child_by_name("light1");
    assert(light1);
    camera = get_owner()->get_child_by_name("相机");
    assert(camera);
    teapot = get_owner()->get_child_by_name("teapot");
    assert(camera);
}
void MoveSystem::on_tick() {
    handle_keyboard(Goonya::Timer::delta());
    handle_mouse();
    
    teapot->rotate_global_axis(Goonya::Vector3f({0, Goonya::Timer::delta() * 0.001f, 0}));
    Goonya::Quaternion r = Goonya::Quaternion::from_eular({Goonya::Timer::delta() * 0.001f, Goonya::Timer::delta() * 0.0015f, 0.0f});
    cube->rotate_local_axis(r);
    light1->set_position({20.0f * sinf(Goonya::Timer::total() * 0.005f), 0.0f, 0.0f});
}
