#include "encoding_cvt.h"

#include <Windows.h>
#include <runtime/GoonyaException.h>

namespace Goonya {

std::wstring utf8_to_wchar(const std::string& utf8) {
    const DWORD kFlags = MB_ERR_INVALID_CHARS;
    std::wstring utf16;
    if (utf8.empty()) return utf16;

    int utf16_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), utf8.length(), NULL, 0);
    
    if (utf16_size == 0)
    {
        throw RuntimeError("字符串转换失败");
    }
    
    utf16.resize(utf16_size);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, &utf16[0], utf16_size);
    return utf16;
}
}