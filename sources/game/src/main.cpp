#include <exception>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <runtime/Goonya.h>
#include <runtime/log/Log.h>
#include <core/world/World.h>
#include <core/input/input.h>
#include <core/timer/timer.h>
#include <filesystem>
#include <Windows.h>

std::filesystem::path get_exe_path(){
    wchar_t szPath[512] = {0};
    GetModuleFileNameW(NULL, szPath, sizeof(szPath) - 1);
    return std::filesystem::path(szPath).remove_filename();
}

class MoveSystem : public ISystem {
private:
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

        if (Goonya::Input::is_key_down(VK_ESCAPE)) {
            PostQuitMessage(0);
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
        if (Goonya::Input::get_key_state(VK_SPACE)) {
            camera->transform.position.y += move_speed;
            camera->is_relat_dirty = true;
        }
        if (Goonya::Input::get_key_state(VK_SHIFT)) {
            camera->transform.position.y -= move_speed;
            camera->is_relat_dirty = true;
        }

        if (Goonya::Input::is_key_down('0')) {
            camera->transform = Transform{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1, 1, 1}};
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
        cube = world.get_root()->get_child_by_name("方块");
        assert(cube);
        lights = world.get_root()->get_child_by_name("lights");
        assert(lights);
        light1 = world.get_root()->get_child_by_name("lights")->get_child_by_name("light1");
        assert(light1);
        camera = world.get_root()->get_child_by_name("相机");
        assert(camera);
    }

    void tick() override {
        handle_keyboard(Goonya::Timer::delta());
        handle_mouse();

        cube->transform.rotation.x += Goonya::Timer::delta() * 0.001f;
        cube->transform.rotation.y += Goonya::Timer::delta() * 0.003f;
        cube->is_relat_dirty = true;
        light1->transform.position.x = 20.0f * sinf(world.get_tick_count() * 0.01f);
        light1->is_relat_dirty = true;
    }

private:
    std::shared_ptr<GObject> cube;
    std::shared_ptr<GObject> lights;
    std::shared_ptr<GObject> light1;
    std::shared_ptr<GObject> camera;
};

int main() {
    try {
        Goonya::init_engine();
        try {
            Json::Value root;
            std::ifstream(get_exe_path() / "../assets/scene1.json") >> root;
            const Json::Value &pointlight = root["pointlights"][0];
            for (const std::string &name : pointlight.getMemberNames()) {
                std::cout << name << std::endl;
            }
        } catch (const Json::Exception &e) {
            LOG_ERROR(e.what());
        }

        world.register_system(new MoveSystem("move"));

        Goonya::main_loop();

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        LOG_ERROR(e.what());
    }

    std::cerr << "正常关闭" << std::endl;

    return 0;
}