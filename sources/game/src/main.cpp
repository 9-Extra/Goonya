#include <exception>
#include <function/world/World.h>
#include <iostream>
#include <memory>
#include <print>
#include <resource/loader/SceneLoader.h>
#include <runtime/Goonya.h>

#include "core/divsions.h"
#include "core/format_exception.h"
#include "core/log/Log.h"
#include "craft/craft.h"
#include "function/animation/Animation.h"
#include "function/components/CpntAnimation.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "logic.h"
#include "resource/ResMng.h"
#include "runtime/GAssert.h"

void init_world(Goonya::World *world) {

    // 从文件加载场景主体
    Ref<Goonya::Scene> scene =
        Goonya::load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了
    for (auto &&obj : scene->nodes) {
        world->get_root()->attach_child(obj);
    }

    // 附加控制器
    std::shared_ptr<Goonya::GObject> controller = std::make_shared<Goonya::GObject>("controller");
    controller->add_component(std::make_unique<MoveSystem>());
    world->get_root()->attach_child(controller);

    std::shared_ptr<Goonya::GObject> k = world->get_root()->get_child_by_path("科拉莉");
    GN_ASSERT(k);
    Ref<Goonya::Animation> animation =
        Goonya::resources.load_resource<Goonya::Animation>("禁止二次配布/龙娘科拉莉2.0/科拉莉:动作 Sour_bone");
    k->create_component<Goonya::CpntAnimator>()->set_animation(animation);
}

int main() {
    try {
        Goonya::init_engine();
        Craft::initalize();

        // 初始捕获鼠标不方便调试
        // Goonya::Display::set_cursor_mode(Goonya::CursorMode::CAPTURED | Goonya::CursorMode::HIDDEN); // 捕获鼠标

        Goonya::World *world = Goonya::World::create_world();
        init_world(world);

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