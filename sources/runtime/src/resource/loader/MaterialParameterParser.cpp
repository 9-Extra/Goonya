#include "MaterialParameterParser.h"
#include "runtime/GoonyaException.h"
#include <regex>
#include <string_view>

namespace Goonya {

MaterialParameter parse_material_parameters(std::string_view parameter_string) {
    // 解析形如"vec3(1.0, 0, 2.0)"这样的参数
    const std::regex pattern(R"(^\s*(\w+)\s*\((.*)\)$)");
    const std::regex number_pattern(R"(\s*([-+]?\d*\.?\d+\s*))");

    using RegexIter = std::regex_iterator<std::string_view::const_iterator>;
    using Match = std::match_results<std::string_view::const_iterator>;

    Match matches;
    if (std::regex_match(parameter_string.begin(), parameter_string.end(), matches, pattern)) {
        // matches[0] 是整个匹配的字符串
        std::string_view type_name = std::string_view(matches[1].first, matches[1].second); // 类型
        std::string_view numbers_str = std::string_view(matches[2].first, matches[2].second);
        RegexIter it(numbers_str.begin(), numbers_str.end(), number_pattern);
        RegexIter end;
        if (type_name == "bool") {
            if (it == end || it->str() == "false") {
                return false;
            } else if (it->str() == "true") {
                return true;
            } else {
                throw RuntimeError(std::format("bool参数格式\"{}\"不正确", it->str()));
            }
        } else if (type_name == "int") {
            if (it == end) {
                return 0;
            }
            int param = std::stoi(it->str());
            ++it;
            GN_ASSERT(it == end);
            return param;
        } else if (type_name == "vec2") {
            if (it == end) {
                return Vector2f{0.0f, 0.0f};
            }
            Vector2f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            GN_ASSERT(it == end);
            return param;
        } else if (type_name == "vec3") {
            if (it == end) {
                return Vector3f{0.0f, 0.0f, 0.0f};
            }
            Vector3f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            GN_ASSERT(it == end);
            return param;
        } else if (type_name == "vec4") {
            if (it == end) {
                return Vector4f{0.0f, 0.0f, 0.0f, 0.0f};
            }
            Vector4f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            GN_ASSERT(it == end);
            return param;
        } else if (type_name == "f32") {
            if (it == end) {
                return 0.0f;
            }
            float param = std::stof(it->str());
            ++it;
            GN_ASSERT(it == end);
            return param;
        } else if (type_name == "mat4") {
            if (it == end) {
                return Matrix4f::zero();
            }
            Matrix4f param{
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str())};

            GN_ASSERT(it == end);
            return param;
        } else {
            throw RuntimeError(std::format("着色器参数类型\"{}\"不支持", type_name));
        }

    } else {
        throw RuntimeError(std::format("着色器参数格式\"{}\"不正确", parameter_string));
    }
}

} // namespace Goonya