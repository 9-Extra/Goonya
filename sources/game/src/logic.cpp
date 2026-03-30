#include "logic.h"

#include "core/cgmath/quaternion.h"
#include "core/clock/GameClock.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/input/input.h"
#include "craft/level/level.h"
#include "function/renderer/Renderer.h"
#include "function/renderer/UberShader.h"
#include "function/world/Component.h"
#include "function/world/World.h"
#include "platform/display/display.h"
#include "runtime/GAssert.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <resource/Resource.h>

void MoveSystem::handle_mouse() const {
    using namespace Goonya;
    if (Goonya::Input::is_mouse_down(Input::LEFT)) {
        // 左键按下时，捕获光标
        Display::set_cursor_mode(CursorMode::CAPTURED | CursorMode::HIDDEN);
    }

    // 如果光标已捕获，根据鼠标移动旋转视角
    // 左上角为(0,0)，右下角为(w,h)
    if (contain(Display::get_cursor_mode(), CursorMode::CAPTURED)) {
        auto [dx, dy] = Input::get_mouse_move();
        // 鼠标向右拖拽，相机沿全局坐标系y轴逆时针旋转。鼠标向下拖拽时，相机沿局部坐标系x轴逆时针旋转
        const float rotate_speed = 0.002f;
        camera->rotate_global_axis({0, -dx * rotate_speed, 0});
        // 同时，要约束绕x轴旋转角度在(-90, 90)度之间
        const float limit = std::numbers::pi_v<float> / 2.0f - 0.01f;
        float y_dir = std::asin(camera->get_local_transform().forward_direction().y);
        float y_dir_target = std::clamp(y_dir - dy * rotate_speed, -limit, limit);
        camera->rotate_local_axis({y_dir_target - y_dir, 0, 0});
    }
}
void MoveSystem::handle_keyboard(float delta) const {
    using namespace Goonya;
    const float move_speed = delta * (Input::is_key_pressing(Input::KeyCode::LCTRL) ? 0.06f : 0.02f);

    // F键开关IBL环境光
    if (Input::is_key_click('F')) {
        const std::string_view key_name = "GYA_IBL_ENVIRONMENT_LIGHT";
        if (!Goonya::GLOBAL_VARIANT_KEY.is_key_set(key_name)) {
            Goonya::GLOBAL_VARIANT_KEY.set_key(key_name);
        } else {
            Goonya::GLOBAL_VARIANT_KEY.reset_key(key_name);
        }
    }

    // R键开关灯
    if (Goonya::Input::is_key_click('R')) {
        if (lights->is_disabled()) {
            lights->enable(true);
        } else {
            lights->disable(true);
        }
    }

    // B键开关Bloom
    if (Goonya::Input::is_key_click('B')) {
        renderer.draw_bloom = !renderer.draw_bloom;
    }

    // 按下LALT时，切换光标模式
    if (Input::is_key_click(Input::KeyCode::LALT)) {
        if (!contain(Display::get_cursor_mode(), CursorMode::CAPTURED)) {
            Display::set_cursor_mode(CursorMode::CAPTURED | CursorMode::HIDDEN);
        } else {
            Display::set_cursor_mode(CursorMode::FREE | CursorMode::VISIBLE);
        }
    }

    if (Goonya::Input::is_key_click(Input::KeyCode::ESCAPE)) {
        EventBus::dispatch_event(Events::EngineStop{});
    }

    // wasd移动
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
    std::shared_ptr<Goonya::GObject> root = get_owner()->get_parent();
    GN_ASSERT(root);
    cube = root->get_child_by_name("方块");
    GN_ASSERT(cube);
    lights = root->get_child_by_name("lights");
    GN_ASSERT(lights);
    light1 = root->get_child_by_name("lights")->get_child_by_name("light1");
    GN_ASSERT(light1);
    camera = root->get_child_by_name("相机");
    GN_ASSERT(camera);

    Goonya::World *world = get_owner()->get_world();
    register_ticker(world);

    level = std::make_unique<Craft::Level>(world, camera);
    level_ticker[0] = std::make_unique<LevelTicker>(level.get());
    level_ticker[1] = std::make_unique<LevelFixedTicker>(level.get());
    for (auto &&t : level_ticker) {
        t->register_ticker(world);
    }
}

void MoveSystem::on_unregister() {
    level.reset();
    level_ticker[0].reset();
    level_ticker[1].reset();
    unregister_ticker();
}

void MoveSystem::tick() {
    float delta_time =
        std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(Goonya::GAME_CLOCK.delta()).count();
    float total_time =
        std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(Goonya::GAME_CLOCK.total()).count();
    handle_keyboard(delta_time);
    handle_mouse();

    Goonya::Quaternion r = Goonya::Quaternion::from_eular({delta_time * 0.001f, delta_time * 0.0015f, 0.0f});
    cube->rotate_local_axis(r);
    light1->set_local_position({20.0f * std::sinf(total_time * 0.005f), 0.0f, 0.0f});
}
