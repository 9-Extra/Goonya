#pragma once

#include "resource/Loader.h"

namespace Goonya {

class GShaderLoader final : public ResourceLoader {
public:
    GShaderLoader() : ResourceLoader({"GUberShader"}) {}

    Ref<Resource> load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                       const Json::Value &content) override;
};

} // namespace Goonya