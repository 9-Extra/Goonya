#include "encoding_cvt.h"

#include <locale>
#include <codecvt>

#include <runtime/GoonyaException.h>


namespace Goonya {

template<class Facet>
struct deletable_facet : Facet
{
    template<class... Args>
    deletable_facet(Args&&... args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};

std::wstring utf8_to_wchar(const std::string& utf8) {
    std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>, wchar_t> conv16;

    std::wstring utf16 = conv16.from_bytes(utf8);
   
    return utf16;
}
}