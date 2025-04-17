#pragma once
#include <filesystem>
#include <string>

namespace Goonya {

std::string read_whole_file(const std::filesystem::path &path);

}
