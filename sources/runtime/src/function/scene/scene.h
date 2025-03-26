#pragma once

#include "core/world/GObject.h"

namespace Goonya{
namespace Scene {

struct Scene{
    std::shared_ptr<GObject> root;
    Scene(){
        root = std::make_shared<GObject>(Transform{}, "root");
    }
};

Scene load_scene_from_json(const std::string &path);
}
}