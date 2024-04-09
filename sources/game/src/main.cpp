#include <exception>
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
#include <core/eventbus/eventbus.h>
#include <function/scene/scene.h>

#include "logic.h"

std::filesystem::path get_exe_path(){
    wchar_t szPath[512] = {0};
    GetModuleFileNameW(NULL, szPath, sizeof(szPath) - 1);
    return std::filesystem::path(szPath).remove_filename();
}

int main() {
    try {
        Goonya::init_engine();

        Goonya::Scene::Scene scene = Goonya::Scene::load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了  
        Goonya::world.get_root().swap(scene.root);

        Goonya::world.register_system(new MoveSystem("move"));

        Goonya::main_loop();

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        std::cerr << "抛出异常：" << e.what() << std::endl;
    }

    std::cerr << "正常关闭" << std::endl;

    return 0;
}