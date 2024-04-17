#include "path.h"

#include <Windows.h>

namespace Goonya{

std::filesystem::path get_exe_path(){
    wchar_t szPath[512] = {0};
    GetModuleFileNameW(NULL, szPath, sizeof(szPath) - 1);
    return std::filesystem::path(szPath).remove_filename();
}
}