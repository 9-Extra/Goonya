#include "read_file.h"
#include "runtime/GoonyaException.h"

#include <Windows.h>

namespace Goonya {
// Mingw的ifstream不知道为什么导致了崩溃，手动实现文件读取
std::string read_whole_file(const std::filesystem::path& path) {
    HANDLE handle = CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        throw Goonya::RuntimeError(std::format("Failed to open file: {}", path.string()));
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size)) {
        throw Goonya::RuntimeError("Failed to get file size");
    }
    std::string chunk;
    chunk.resize(size.QuadPart);

    char *ptr = (char *)chunk.data();
    char *end = ptr + chunk.size();
    DWORD read = 0;
    while (ptr < end) {
        DWORD to_read = (DWORD)std::min<size_t>(std::numeric_limits<DWORD>::max(), end - ptr);
        if (!ReadFile(handle, ptr, to_read, &read, NULL)) {
            throw Goonya::RuntimeError("Failed to read file");
        }
        ptr += read;
    }

    CloseHandle(handle);

    return chunk;
}
}