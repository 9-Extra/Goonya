#pragma once

#include <string>
#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

namespace Goonya {

enum class ThreadType {
    UNKNOWN = 0,
    LOGIC = 1,
    RENDER = 2,
    WORKER = 3,
};

inline thread_local ThreadType current_thread_type = ThreadType::UNKNOWN;

inline void set_current_thread_name(const std::string &name) {
#if defined(_GNU_SOURCE) && (((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 12))))
    // Linux平台实现
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
#elif defined(_WIN32)
    // Windows
    int len = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
    wchar_t *buffer = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, buffer, len);

    SetThreadDescription(GetCurrentThread(), buffer);
    delete[] buffer;
#else
    // 平台不支持
#endif
}

} // namespace Goonya
