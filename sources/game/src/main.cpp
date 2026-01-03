#include <exception>
#include <function/world/World.h>
#include <iostream>
#include <memory>
#include <print>
#include <resource/loader/SceneLoader.h>
#include <runtime/Goonya.h>

#include "core/format_exception.h"
#include "core/log/Log.h"
#include "core/divsions.h"
#include "craft/craft.h"
#include "function/world/GObject.h"
#include "logic.h"

int main() {
    try {
        Goonya::init_engine();
        Craft::initalize();

        Goonya::World* world = Goonya::World::create_world();

        {
            Ref<Goonya::Scene> scene =
                Goonya::load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了
            world->set_root(scene->root);
        }
        // 初始捕获鼠标不方便调试
        // Goonya::Display::set_cursor_mode(Goonya::CursorMode::CAPTURED | Goonya::CursorMode::HIDDEN); // 捕获鼠标
        
        world->get_root()->add_component(std::make_unique<MoveSystem>());

        Goonya::main_loop();

        Goonya::World::delete_world(world);

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        LOG_ERROR(Goonya::format_exception(e));
        Goonya::drop_engine();
    }

    std::println(std::cerr, "正常关闭");

    return 0;
}