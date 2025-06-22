#pragma once

#include "core/world/GObject.h"

namespace Goonya::Scene {

struct Scene{
    std::string name;
    std::shared_ptr<GObject> root;
};

Scene load_scene_from_json(const std::string &path);
}
