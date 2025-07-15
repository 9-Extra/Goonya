#include "read_file.h"
#include "runtime/GoonyaException.h"

#include <format>
#include <fstream> // IWYU pragma: keep

namespace Goonya {

std::string read_whole_file(const std::filesystem::path &path) {
    std::filebuf fb;
    fb.open(path, std::ios::in | std::ios::binary);
    if (!fb.is_open()) {
        throw RuntimeError(std::format("Fail to open file {}", path.string()));
    }

    // Obtain the size of the file.
    const auto sz = std::filesystem::file_size(path);

    // Create a buffer.
    std::string result(sz, '\0');

    // Read the whole file into the buffer.
    fb.sgetn(result.data(), sz);

    return result;
}
} // namespace Goonya