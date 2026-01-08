#pragma once

#include "resource/Loader.h"
#include "resource/Resource.h"

#include <filesystem>

namespace Goonya {

class GlTFLoader : public ResourceLoader {
public:
    GlTFLoader() : ResourceLoader({"glTF"}) {}

protected:
    Ref<Resource> load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                       const Json::Value &content) override;
};

} // namespace Goonya
