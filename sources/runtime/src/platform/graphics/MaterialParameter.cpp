#include "MaterialParameter.h"

namespace Goonya {

std::string_view get_type_name_glsl(const MaterialParameter &p) noexcept {
    switch (p.index()) {
    case 0:
        return "";
    case 1:
        return "bool";
    case 2:
        return "int";
    case 3:
        return "float";
    case 4:
        return "vec2";
    case 5:
        return "vec3";
    case 6:
        return "vec4";
    case 7:
        return "mat3";
    case 8:
        return "mat4";
    default:
        return "";
    }
}
} // namespace Goonya
