#include "logic.h"

#include "core/RefCount.h"
#include "core/cgmath/quaternion.h"
#include "core/clock/GameClock.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/input/input.h"
#include "core/log/Log.h"
#include "craft/level/level.h"
#include "function/world/Component.h"
#include "function/world/World.h"
#include "platform/display/display.h"
#include "resource/ResMng.h"

#include <platform/graphics/Shader.h>
#include <resource/Resource.h>

void MoveSystem::handle_mouse() const {
    using namespace Goonya;
    if (Goonya::Input::is_mouse_click(Input::LEFT)) {
        lights->enable();
        // 左键点击时，捕获光标
        Display::set_cursor_mode(CursorMode::CAPTURED | CursorMode::HIDDEN);
    }
    if (Goonya::Input::is_mouse_click(Input::RIGHT)) {
        lights->disable();
    }

    // 如何光标已捕获，使用鼠标移动旋转视角
    // 左上角为(0,0)，右下角为(w,h)
    if (contain(Display::get_cursor_mode(), CursorMode::CAPTURED)) {
        auto [dx, dy] = Input::get_mouse_move();
        // 鼠标向右拖拽，相机沿全局坐标系y轴逆时针旋转。鼠标向下拖拽时，相机沿局部坐标系x轴逆时针旋转
        const float rotate_speed = 0.006f;
        camera->rotate_local_axis({-dy * rotate_speed, 0, 0});
        camera->rotate_global_axis({0, -dx * rotate_speed, 0});
    }
}
void MoveSystem::handle_keyboard(float delta) const {
    // wasd移动
    using namespace Goonya;
    const float move_speed = delta * (Input::is_key_pressing(Input::KeyCode::LCTRL) ? 0.06f : 0.02f);

    if (Input::is_key_click('F')) {
        const std::string_view key_name = "GYA_IBL_ENVIRONMENT_LIGHT";
        Graphics::ShaderLib *shader_lib = resources.shader_lib.get();
        if (!shader_lib->is_global_variant_key_set(key_name)) {
            shader_lib->set_global_variant_key(key_name);
        } else {
            shader_lib->reset_global_variant_key(key_name);
        }
    }

    // 按下LALT时，切换光标模式
    if (Input::is_key_click(Input::KeyCode::LALT)){
        if (!contain(Display::get_cursor_mode(), CursorMode::CAPTURED)) {
            Display::set_cursor_mode(CursorMode::CAPTURED | CursorMode::HIDDEN);
        } else {
            Display::set_cursor_mode(CursorMode::FREE | CursorMode::VISIBLE);
        }
    }

    if (Input::is_key_click('P')) {
        LOG_DEBUG("正在进行图像导出");
        Ref<Graphics::Texture> skybox = resources.cubemaps.get("skybox_valley_color");
        stb::Image image = skybox->export_image(0);
        image.save("output.hdr");
    }

    if (Goonya::Input::is_key_click(Input::KeyCode::ESCAPE)) {
        EventBus::dispatch_event(Events::EngineStop{});
    }

    const Transform &trans = camera->get_local_transform();
    if (Input::is_key_pressing('W')) {
        Vector3f ori = trans.forward_direction();
        ori.y = 0.0;
        camera->translate_local(ori.normalize() * move_speed);
    }
    if (Input::is_key_pressing('S')) {
        Vector3f ori = trans.forward_direction();
        ori.y = 0.0;
        camera->translate_local(ori.normalize() * -move_speed);
    }
    if (Input::is_key_pressing('A')) {
        Vector3f ori = trans.forward_direction();
        ori = {ori.z, 0.0, -ori.x};
        camera->translate_local(ori.normalize() * move_speed);
    }
    if (Input::is_key_pressing('D')) {
        Vector3f ori = trans.forward_direction();
        ori = {ori.z, 0.0, -ori.x};
        camera->translate_local(ori.normalize() * -move_speed);
    }
    if (Input::is_key_pressing(Input::KeyCode::SPACE)) {
        camera->translate_local({0.0f, move_speed, 0.0f});
    }
    if (Input::is_key_pressing(Input::KeyCode::LSHIFT)) {
        camera->translate_local({0.0f, -move_speed, 0.0f});
    }

    if (Input::is_key_click('0')) {
        camera->set_local_transform(Transform{});
    }

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
    assert(teapot);

    Goonya::World* world = get_owner()->get_world();
    world->register_ticker(this);
    level = create_ref<Craft::Level>(get_owner()->get_world(), camera);
    world->register_ticker(level.get());
}

void MoveSystem::on_unregister() { 
    Goonya::World* world = get_owner()->get_world();
    world->unregister_ticker(this);
    world->unregister_ticker(level.get());
}

void MoveSystem::tick() {
    float delta_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(Goonya::GAME_CLOCK.delta()).count();
    float total_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(Goonya::GAME_CLOCK.total()).count();
    handle_keyboard(delta_time);
    handle_mouse();

    teapot->rotate_global_axis(Goonya::Vector3f({0, delta_time * 0.001f, 0}));
    Goonya::Quaternion r =
        Goonya::Quaternion::from_eular({delta_time * 0.001f, delta_time * 0.0015f, 0.0f});
    cube->rotate_local_axis(r);
    light1->set_local_position({20.0f * std::sinf(total_time * 0.005f), 0.0f, 0.0f});
}
