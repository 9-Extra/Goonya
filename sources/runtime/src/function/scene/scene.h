#pragma once

#include "core/world/GObject.h"

namespace Goonya{
namespace Scene {

struct Scene{
    std::shared_ptr<GObject> root;
    Scene(){
        root = std::make_shared<GObject>(Transform{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, "root");
    }
};

Scene load_scene_from_json(const std::string &path);
}
}