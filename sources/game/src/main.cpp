#include <core/world/World.h>
#include <exception>
#include <function/scene/scene.h>
#include <nowide/iostream.hpp>
#include <runtime/Goonya.h>

#include "logic.h"

int main() {
    try {
        Goonya::init_engine();

        {
            Goonya::Scene::Scene scene =
                Goonya::Scene::load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了
            Goonya::world.root = scene.root;
        }

        Goonya::world.root->add_component(std::make_unique<MoveSystem>());

        Goonya::main_loop();

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        nowide::cerr << "抛出异常：" << e.what() << std::endl;
    }

    nowide::cerr << "正常关闭" << std::endl;

    return 0;
}