#include "MaterialParameterParser.h"
#include "runtime/GoonyaException.h"
#include <regex>

namespace Goonya {

MaterialParameter parse_material_parameters(const std::string &parameter_string) {
    // 解析形如"vec3(1.0, 0, 2.0)"这样的参数
    const std::regex pattern(R"(^\s*(\w+)\s*\((.*)\)$)");
    const std::regex number_pattern(R"(\s*([-+]?\d*\.?\d+\s*))");
    std::smatch matches;
    if (std::regex_match(parameter_string, matches, pattern)) {
        // matches[0] 是整个匹配的字符串
        std::string type_name = matches[1].str(); // 类型
        std::string numbers_str = matches[2].str();
        std::sregex_iterator it(numbers_str.begin(), numbers_str.end(), number_pattern);
        std::sregex_iterator end;
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
            assert(it == end);
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
            assert(it == end);
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
            assert(it == end);
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
            assert(it == end);
            return param;
        } else if (type_name == "f32") {
            if (it == end) {
                return 0.0f;
            }
            float param = std::stof(it->str());
            ++it;
            assert(it == end);
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

            assert(it == end);
            return param;
        } else {
            throw RuntimeError(std::format("着色器参数类型\"{}\"不支持", type_name));
        }

    } else {
        throw RuntimeError(std::format("着色器参数格式\"{}\"不正确", parameter_string));
    }
}

} // namespace Goonya