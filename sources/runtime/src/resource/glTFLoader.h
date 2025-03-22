#pragma once

#include <string>
#include <filesystem>

namespace Goonya {
namespace Resource {

void load_gltf(const std::string &base_key, const std::filesystem::path &path);

}
}