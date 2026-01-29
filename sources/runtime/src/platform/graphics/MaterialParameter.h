#pragma once

#include <variant>

#include "core/cgmath/cgmath.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"

namespace Goonya {

using MaterialParameter =
    std::variant<std::monostate, bool, int, float, Vector2f, Vector3f, Vector4f, Matrix3f, Matrix4f>;
static_assert(std::equality_comparable<MaterialParameter>);

std::string_view get_type_name_glsl(const MaterialParameter &p) noexcept;
} // namespace Goonya
