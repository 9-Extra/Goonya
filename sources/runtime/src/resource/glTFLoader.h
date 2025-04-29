#pragma once

#include "core/assets.h"

#include <filesystem>


namespace Goonya::Resource {

void load_gltf(const AssetKey &base_key, const std::filesystem::path &path);

}
