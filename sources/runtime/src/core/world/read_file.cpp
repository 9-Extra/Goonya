#include <algorithm>
#include <iostream>

#include <Windows.h>


// Mingw的ifstream不知道为什么导致了崩溃，手动实现文件读取
std::string read_whole_file(const std::string &path) {
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file: " << path << std::endl;
        exit(-1);
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size)) {
        std::cerr << "Failed to get file size\n";
        exit(-1);
    }
    std::string chunk;
    chunk.resize(size.QuadPart);

    char *ptr = (char *)chunk.data();
    char *end = ptr + chunk.size();
    DWORD read = 0;
    while (ptr < end) {
        DWORD to_read = (DWORD)std::min<size_t>(std::numeric_limits<DWORD>::max(), end - ptr);
        if (!ReadFile(handle, ptr, to_read, &read, NULL)) {
            std::cerr << "Failed to read file\n";
            exit(-1);
        }
        ptr += read;
    }

    CloseHandle(handle);

    return chunk;
}