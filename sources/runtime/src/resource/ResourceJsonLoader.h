#pragma once

#include <string>

namespace Goonya {
namespace Resource {

void load_gltf(const std::string &base_key, const std::string &path);
void load_json(const std::string &path);

}
}