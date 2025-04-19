#include "Shader.h"
#include "core/log/Log.h"
#include "platform/graphics/Graphics.h"
#include "resource/Resource.h"
#include <memory>

namespace Goonya::Graphics {

void VariantKeyCollect::add_variant_key_group(std::vector<std::string> &&group_keys) {
    if (group_keys.size() < 2) {
        throw RuntimeError("每个组至少有2个成员");
    }

    uint8_t group_index = variants_key_map.size();
    uint8_t index = 0;
    for (const std::string &key : group_keys) {
        if (!key.empty()) { // 略过空定义
            auto [_, inserted] = variants_key_map.emplace(key, std::make_tuple(group_index, index));
            if (!inserted) {
                throw RuntimeError("着色器变体键重复定义");
            }
        }
        index++;
    }

    uint32_t group_key_count = group_keys.size();
    uint32_t next_group_base = variant_count;
    variants_key_groups.emplace_back(next_group_base, std::move(group_keys));
    variant_count *= group_key_count;
    assert(variant_count >= group_key_count); // 越界检测
}
void VariantKeyCollect::get_variant_key_names(VariantCode code, std::vector<std::string> &out_result) const noexcept {
    for (const auto &[base, group] : variants_key_groups) {
        const std::string &key = group[code / base % static_cast<uint32_t>(group.size())];
        if (!key.empty()) { // 空字符串视为不进行定义
            out_result.emplace_back(key);
        }
    }
}
bool VariantKeyCollect::set_variant_code(VariantCode &code, const std::string &variant_key) const noexcept {
    if (auto iter = variants_key_map.find(variant_key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        uint32_t current_index = code / base % static_cast<uint32_t>(group.size()); // 当前本组选择的变体下标
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
bool VariantKeyCollect::reset_variant_code(VariantCode &code, const std::string &variant_key) const noexcept {
    if (auto iter = variants_key_map.find(variant_key); iter != variants_key_map.end()) {
        auto [group_index, _] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        uint32_t current_index = code / base % static_cast<uint32_t>(group.size()); // 当前本组选择的变体下标
        if (current_index != 0) {
            code -= base * current_index;
            return true;
        }
        return false;
    } else {
        return false;
    }
}
bool VariantKeyCollect::is_key_defined(VariantCode code, const std::string &key) const noexcept {
    if (auto iter = variants_key_map.find(key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        return code / base % static_cast<uint32_t>(group.size()) == index;
    }
    return false;
}

/**
 * @brief 将宏定义等代码插入到着色器中以生成对应变体
 * 插入的代码将替换着色器源码中的#pragma GYA_INJECT
 * @param src 着色器代码
 * @param variant_key
 * @return 生成的代码
 */
static std::string shader_source_inject(const std::string &src, const std::vector<std::string> &variant_key) {

    const std::string PATTERN = "#pragma GYA_INJECT";

    std::stringstream ss;
    size_t injection_point = src.find(PATTERN);
    if (injection_point == std::string::npos) {
        LOG_WARN("着色器中未找到\"{}\"，无法正确进行变体生成", PATTERN);
    }
    ss << src.substr(0, injection_point);

    ss << "//------Combined Definitions---------: \n";

    for (const std::string &key : variant_key) {
        ss << std::format("#ifdef {0}\n#undef {0}\n#endif\n#define {0}\n", key);
    }

    ss << "//------Combined Definition End------: \n";

    ss << src.substr(injection_point + PATTERN.size());

    return ss.str();
}

UberShader::UberShader(UberShaderDesc &&desc) {
    this->vs_src = std::move(desc.vs_src);
    this->ps_src = std::move(desc.ps_src);
    this->global_variant_key_collect = VariantKeyCollect(std::move(desc.global_variant_keys));
    this->local_variant_key_collect = VariantKeyCollect(std::move(desc.local_variant_keys));

    // 立即编译一个不包含任何变体的版本用于反射，此时uber_shader不完整，不能用create_variant
    VariantCodeSet empty{0};
    std::vector<std::string> variant_keys;
    this->get_variant_key_names(empty, variant_keys);
    variant_keys.emplace_back("VERTEX_SHADER");
    std::string mixed_vs = shader_source_inject(this->vs_src, variant_keys);
    variant_keys.back() = "PIXEL_SHADER";
    std::string mixed_ps = shader_source_inject(this->ps_src, variant_keys);

    intrusive_ptr<Shader> shader = graphics_api->compile_shader_program(mixed_vs, mixed_ps);
    this->shaders[empty] = shader; // 既然编译了就加入缓存

    // 反射获取着色器信息
    std::unique_ptr<ShaderIntrospector> introspector = graphics_api->create_shader_introspect(shader.get());
    auto buffer_info = introspector->get_constant_buffer_info();

    this->per_material = std::move(buffer_info["per_material"]);
    this->per_frame = std::move(buffer_info.at("per_frame"));
    this->per_object = std::move(buffer_info["per_object"]);
    this->texture_units = introspector->get_texture_info();

    this->global_key_code = 0; // 记得初始化为0！
    // 设置现有的全局变体键
    for (const std::string &key : Resource::resources.shader_lib->get_global_variant_keys()) {
        this->global_variant_key_collect.set_variant_code(this->global_key_code, key);
    }
}

void ShaderLib::add_uber_shader(const AssetKey &name, UberShaderDesc &&desc) {
    uber_shaders.emplace(name, new UberShader{std::move(desc)});
}

intrusive_ptr<Shader> UberShader::query_variant(VariantCodeSet variant_code) {
    if (auto iter = shaders.find(variant_code); iter != shaders.end()) {
        return iter->second;
    }

    std::vector<std::string> variant_keys;
    get_variant_key_names(variant_code, variant_keys);
    std::string mixed_vs = shader_source_inject(vs_src, variant_keys);
    std::string mixed_ps = shader_source_inject(ps_src, variant_keys);

    intrusive_ptr<Shader> shader = graphics_api->compile_shader_program(mixed_vs, mixed_ps);
    shaders.emplace(variant_code, shader);

    return shader;
}
} // namespace Goonya::Graphics
