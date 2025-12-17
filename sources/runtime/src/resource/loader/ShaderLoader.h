#pragma once

#include "resource/Loader.h"

namespace Goonya {

class ShaderLoader final : public ResourceLoader {
public:
    ShaderLoader() : ResourceLoader({"UberShader"}) {}

    Ref<Resource> load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                       const Json::Value &content) override;
};

} // namespace Goonya