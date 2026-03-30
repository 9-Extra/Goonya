#pragma once

#include "core/as_u8string.h"
#include "function/world/GObject.h"
#include "resource/Loader.h"
#include "resource/Resource.h"
#include "runtime/GoonyaException.h"
#include <filesystem>

namespace Goonya {

struct Scene : public Resource {
    std::string name;
    std::vector<std::shared_ptr<GObject>> nodes; // 根节点可能不止一个
};

Ref<Scene> load_scene_from_json(const std::filesystem::path &path);

class SceneLoader : public ResourceLoader {
public:
    SceneLoader() : ResourceLoader({"scene"}) {}

protected:
    Ref<Resource> load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                       const Json::Value &content) override {
        if (!content.isMember("file")) {
            throw RuntimeError("字段file缺失");
        }
        return load_scene_from_json(base_dir / as_u8string_view(content["file"].asString()));
    };
};

} // namespace Goonya
