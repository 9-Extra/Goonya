#pragma once

#include "resource/Loader.h"

namespace Goonya {

class MateriaLoader : public ResourceLoader {
public:
    MateriaLoader() : ResourceLoader({"Material"}) {}

protected:
    Ref<Resource> load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                       const Json::Value &content) override;
};
} // namespace Goonya