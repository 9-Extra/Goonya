#include "Shader.h"

namespace Goonya {
namespace Graphics {

void ShaderVariantKeyCollect::add_variant_key_group(std::vector<std::string> &&group_keys) {
    uint8_t group_index = variants_key_map.size();
    uint8_t index = 0;
    for (const std::string &key : group_keys) {
        auto [_, inserted] = variants_key_map.emplace(key, std::make_tuple(group_index, index++));
        if (!inserted) {
            throw RuntimeError("着色器变体键重复定义");
        }
    }

    uint32_t group_key_count = group_keys.size();
    uint32_t next_group_base = varient_count;
    variants_key_groups.emplace_back(next_group_base, std::move(group_keys));
    varient_count *= group_key_count;
    assert(varient_count >= group_key_count); // 越界检测
}
void ShaderVariantKeyCollect::get_variant_key_names(VariantCode code,
                                                    std::vector<std::string> &out_result) const noexcept {
    for (const auto &[base, group] : variants_key_groups) {
        const std::string &key = group[code / base % (uint32_t)group.size()];
        if (key.size() != 0) { // 空字符串视为不进行定义
            out_result.emplace_back(key);
        }
    }
}
bool ShaderVariantKeyCollect::set_varient_code(VariantCode &code, const std::string &varient_key) const noexcept {
    if (auto iter = variants_key_map.find(varient_key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        uint32_t current_index = code / base % (uint32_t)group.size(); // 当前本组选择的变体下标
        if (current_index != index) {
            code += base * index - base * current_index;
            return true;
        }
        return false;
    } else {
        // 如果此键不存在，也返回false
        return false;
    }
}
bool ShaderVariantKeyCollect::reset_varient_code(VariantCode &code, const std::string &varient_key) const noexcept {
    if (auto iter = variants_key_map.find(varient_key); iter != variants_key_map.end()) {
        auto [group_index, _] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        uint32_t current_index = code / base % (uint32_t)group.size(); // 当前本组选择的变体下标
        if (current_index != 0) {
            code -= base * current_index;
            return true;
        }
        return false;
    } else {
        return false;
    }
}
bool ShaderVariantKeyCollect::is_key_defined(VariantCode code, const std::string &key) const noexcept {
    if (auto iter = variants_key_map.find(key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        return code / base % (uint32_t)group.size() == index;
    }
    return false;
}
} // namespace Graphics
} // namespace Goonya