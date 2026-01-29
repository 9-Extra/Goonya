#pragma once

#include "platform/graphics/MaterialParameter.h"

namespace Goonya {
/**
 * @brief 解析形如"vec3(1.0, 0, 2.0)"这样的材质参数，对"vec3()"格式取默认值
 *
 * @param parameter_string 字符串格式的参数
 * @return MaterialParameter 解析后的参数
 */
MaterialParameter parse_material_parameters(std::string_view parameter_string);
} // namespace Goonya