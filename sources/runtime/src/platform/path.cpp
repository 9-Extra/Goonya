#include "path.h"
#include "runtime/GoonyaException.h"
#include <filesystem>
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#include <Windows.h>
#elif __linux__
#include <linux/limits.h>
#include <unistd.h>
#include <sys/stat.h>
#endif


namespace Goonya{

std::filesystem::path get_exe_path(){
#ifdef _WIN32
    wchar_t szPath[512] = {0};
    GetModuleFileNameW(NULL, szPath, sizeof(szPath) - 1);
    return std::filesystem::path(szPath).remove_filename();
#elif __linux__
    const char link[] = "/proc/self/exe";
    struct stat sb;
    int ret = lstat(link, &sb);
    size_t buf_size = PATH_MAX;
    if (ret != -1 && sb.st_size != 0){
        buf_size = sb.st_size;
    }
    std::string buf(0, buf_size);
    ssize_t nbytes = readlink(link, buf.data(), buf_size);
    if (nbytes == -1){
        throw RuntimeError("读取路径出错");
    }
    return std::filesystem::path(buf);

#else
    # error "Unsupported platform"
#endif
}
}