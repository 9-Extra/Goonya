#include "MaterialParameter.h"

#include <type_traits>
#include <variant>

namespace Goonya {

std::string_view get_type_name_glsl(const MaterialParameter &p) noexcept {
    return std::visit(
        [](const auto &v) -> std::string_view {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>) {
                return "int";
            } else if constexpr (std::is_same_v<T, float>) {
                return "float";
            } else if constexpr (std::is_same_v<T, Vector2f>) {
                return "vec2";
            } else if constexpr (std::is_same_v<T, Vector3f>) {
                return "vec3";
            } else if constexpr (std::is_same_v<T, Vector4f>) {
                return "vec4";
            } else if constexpr (std::is_same_v<T, Matrix4f>) {
                return "mat4";
            } else {
                return "";
            }
        },
        p);
}
} // namespace Goonya
