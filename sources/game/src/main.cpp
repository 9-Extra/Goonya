#include <core/world/World.h>
#include <exception>
#include <iostream>
#include <memory>
#include <print>
#include <resource/scene/scene.h>
#include <runtime/Goonya.h>

#include "core/world/GObject.h"
#include "logic.h"
#include "resource/ResMng.h"

int main() {
    try {
        Goonya::init_engine();

        {
            Goonya::Scene::Scene scene =
                Goonya::Scene::load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了
            std::shared_ptr<Goonya::GObject> k = Goonya::resources.scenes.at("科拉莉.Scene").root;
            k->set_position({2, 0, 0});
            scene.root->attach_child(k);
            Goonya::world.root = std::move(scene.root);
        }

        Goonya::world.root->add_component(std::make_unique<MoveSystem>());

        Goonya::main_loop();

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        std::println(std::cerr, "抛出异常{}: {}", typeid(e).name(), e.what());
    }

    std::println(std::cerr, "正常关闭");

    return 0;
}