#pragma once
#include <core/world/GObject.h>
#include <core/input/input.h>
#include <core/eventbus/eventbus.h>
#include <core/events.h>
#include <core/timer/timer.h>

class MoveSystem : public Goonya::Component {
public:
    void handle_mouse() {
       
        if (Goonya::Input::is_mouse_down(Goonya::Input::LEFT)) {
            lights->enable();
        }
        if (Goonya::Input::is_mouse_down(Goonya::Input::RIGHT)) {
            lights->disable();
        }

        // 使用鼠标中键旋转视角
        //  左上角为(0,0)，右下角为(w,h)
        if (Goonya::Input::get_mouse_state(Goonya::Input::MIDDLE)) {
            auto [dx, dy] = Goonya::Input::get_mouse_move();
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            camera->rotate({0.0f, -dy * rotate_speed, dx * rotate_speed});
        }

    }

    void handle_keyboard(float delta) {
        // wasd移动
        const float move_speed = 0.02f * delta;

        if (Goonya::Input::is_key_down(Goonya::Input::KeyCode::ESCAPE)) {
            Goonya::EventBus::dispatch_event(Goonya::Events::EngineStop{});
        }
        const Goonya::Transform& trans = camera->get_transform();
        if (Goonya::Input::get_key_state('W')) {
            Vector3f ori = trans.get_orientation();
            ori.y = 0.0;
            camera->translate(ori.normalize() * move_speed);
        }
        if (Goonya::Input::get_key_state('S')) {
            Vector3f ori = trans.get_orientation();
            ori.y = 0.0;
            camera->translate(ori.normalize() * -move_speed);
        }
        if (Goonya::Input::get_key_state('A')) {
            Vector3f ori = trans.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            camera->translate(ori.normalize() * move_speed);
        }
        if (Goonya::Input::get_key_state('D')) {
            Vector3f ori = trans.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            camera->translate(ori.normalize() * -move_speed);
        }
        if (Goonya::Input::get_key_state(Goonya::Input::KeyCode::SPACE)) {
            camera->translate({0.0f, move_speed, 0.0f});
        }
        if (Goonya::Input::get_key_state(Goonya::Input::KeyCode::LSHIFT)) {
            camera->translate({0.0f, -move_speed, 0.0f});
        }

        if (Goonya::Input::is_key_down('0')) {
            camera->set_transform(Goonya::Transform{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1, 1, 1}});
        }

        // if (Goonya::Input::is_key_down(VK_UP)) {
        //     renderer.fog_density += 0.00001f * delta;
        // }

        // if (Goonya::Input::is_key_down(VK_DOWN)) {
        //     renderer.fog_density -= 0.00001f * delta;
        //     renderer.fog_density = std::max(renderer.fog_density, 0.0f);
        // }
    }

    virtual void attach() override {
        cube = get_owner()->get_child_by_name("方块");
        assert(cube);
        lights = get_owner()->get_child_by_name("lights");
        assert(lights);
        light1 = get_owner()->get_child_by_name("lights")->get_child_by_name("light1");
        assert(light1);
        camera = get_owner()->get_child_by_name("相机");
        assert(camera);
    }

    virtual void tick() override {
        handle_keyboard(Goonya::Timer::delta());
        handle_mouse();

        cube->rotate({Goonya::Timer::delta() * 0.001f, Goonya::Timer::delta() * 0.003f, 0.0f});
        light1->set_position({20.0f * sinf(Goonya::Timer::total() * 0.005f), 0.0f, 0.0f});
    }

private:
    std::shared_ptr<Goonya::GObject> cube;
    std::shared_ptr<Goonya::GObject> lights;
    std::shared_ptr<Goonya::GObject> light1;
    std::shared_ptr<Goonya::GObject> camera;
};