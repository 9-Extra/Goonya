#pragma once

#include "function/world/GObject.h"
#include "resource/Resource.h"

namespace Goonya::Scene {

struct Scene : public Resource {
    std::string name;
    std::shared_ptr<GObject> root;
};

Ref<Scene> load_scene_from_json(const std::string &path);
} // namespace Goonya::Scene
