#include <exception>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <runtime/Goonya.h>
#include <runtime/log/Log.h>
#include <core/world/World.h>
#include <core/input/input.h>
#include <core/timer/timer.h>
#include <core/eventbus/eventbus.h>
#include <function/scene/scene.h>

#include "logic.h"

int main() {
    try {
        Goonya::init_engine();

        Goonya::Scene::Scene scene = Goonya::Scene::load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了  
        Goonya::world.get_root().swap(scene.root);

        Goonya::world.get_root()->add_component(std::make_unique<MoveSystem>());

        Goonya::main_loop();

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        std::cerr << "抛出异常：" << e.what() << std::endl;
    }

    std::cerr << "正常关闭" << std::endl;

    return 0;
}