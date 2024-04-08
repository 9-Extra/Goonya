#pragma once
#include <core/world/World.h>
#include <core/input/input.h>
#include <core/eventbus/eventbus.h>
#include <core/events.h>
#include <core/timer/timer.h>

class MoveSystem : public Goonya::ISystem {
public:
    using ISystem::ISystem;
    void handle_mouse() {
        // 使用鼠标中键旋转视角
        //  左上角为(0,0)，右下角为(w,h)
        static bool is_left_button_down = false;
        static bool is_right_button_down = false;

        if (Goonya::Input::is_mouse_down(Goonya::Input::LEFT)) {
            lights->enable();
        }

        if (Goonya::Input::is_mouse_down(Goonya::Input::RIGHT)) {
            lights->disable();
        }

        if (Goonya::Input::get_mouse_state(Goonya::Input::MIDDLE)) {
            auto [dx, dy] = Goonya::Input::get_mouse_move();
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            Vector3f &rotation = camera->transform.rotation;
            rotation.z += dx * rotate_speed;
            rotation.y -= dy * rotate_speed;
            camera->is_relat_dirty = true;
        }

    }

    void handle_keyboard(float delta) {
        // wasd移动
        const float move_speed = 0.02f * delta;

        if (Goonya::Input::is_key_down(Goonya::Input::KeyCode::ESCAPE)) {
            Goonya::EventBus::dispatch_event(Goonya::Events::EngineStop{});
        }
        if (Goonya::Input::get_key_state('W')) {
            Vector3f ori = camera->transform.get_orientation();
            ori.y = 0.0;
            camera->transform.position += ori.normalize() * move_speed;
            camera->is_relat_dirty = true;
        }
        if (Goonya::Input::get_key_state('S')) {
            Vector3f ori = camera->transform.get_orientation();
            ori.y = 0.0;
            camera->transform.position += ori.normalize() * -move_speed;
            camera->is_relat_dirty = true;
        }
        if (Goonya::Input::get_key_state('A')) {
            Vector3f ori = camera->transform.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            camera->transform.position += ori.normalize() * move_speed;
            camera->is_relat_dirty = true;
        }
        if (Goonya::Input::get_key_state('D')) {
            Vector3f ori = camera->transform.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            camera->transform.position += ori.normalize() * -move_speed;
            camera->is_relat_dirty = true;
        }
        if (Goonya::Input::get_key_state(Goonya::Input::KeyCode::SPACE)) {
            camera->transform.position.y += move_speed;
            camera->is_relat_dirty = true;
        }
        if (Goonya::Input::get_key_state(Goonya::Input::KeyCode::LSHIFT)) {
            camera->transform.position.y -= move_speed;
            camera->is_relat_dirty = true;
        }

        if (Goonya::Input::is_key_down('0')) {
            camera->transform = Goonya::Transform{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1, 1, 1}};
            camera->is_relat_dirty = true;
        }

        // if (Goonya::Input::is_key_down(VK_UP)) {
        //     renderer.fog_density += 0.00001f * delta;
        // }

        // if (Goonya::Input::is_key_down(VK_DOWN)) {
        //     renderer.fog_density -= 0.00001f * delta;
        //     renderer.fog_density = std::max(renderer.fog_density, 0.0f);
        // }
    }

    void on_attach() override {
        cube = Goonya::world.get_root()->get_child_by_name("方块");
        assert(cube);
        lights = Goonya::world.get_root()->get_child_by_name("lights");
        assert(lights);
        light1 = Goonya::world.get_root()->get_child_by_name("lights")->get_child_by_name("light1");
        assert(light1);
        camera = Goonya::world.get_root()->get_child_by_name("相机");
        assert(camera);
    }

    void tick() override {
        handle_keyboard(Goonya::Timer::delta());
        handle_mouse();

        cube->transform.rotation.x += Goonya::Timer::delta() * 0.001f;
        cube->transform.rotation.y += Goonya::Timer::delta() * 0.003f;
        cube->is_relat_dirty = true;
        light1->transform.position.x = 20.0f * sinf(Goonya::world.get_tick_count() * 0.01f);
        light1->is_relat_dirty = true;
    }

private:
    std::shared_ptr<Goonya::GObject> cube;
    std::shared_ptr<Goonya::GObject> lights;
    std::shared_ptr<Goonya::GObject> light1;
    std::shared_ptr<Goonya::GObject> camera;
};