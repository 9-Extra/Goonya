#include "read_file.h"
#include "runtime/GoonyaException.h"

#include <format>
#include <fstream>

namespace Goonya {

std::string read_whole_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f){
        throw RuntimeError(std::format("Fail to open file {}", path.string()));
    }

    // Obtain the size of the file.
    const auto sz = std::filesystem::file_size(path);

    // Create a buffer.
    std::string result(sz, '\0');

    // Read the whole file into the buffer.
    f.read(result.data(), sz);

    return result;
}
}