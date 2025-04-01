#include "Shader.h"
#include "core/log/Log.h"
#include "platform/graphics/Graphics.h"
#include <memory>

namespace Goonya {
namespace Graphics {

void VariantKeyCollect::add_variant_key_group(std::vector<std::string> &&group_keys) {
    uint8_t group_index = variants_key_map.size();
    uint8_t index = 0;
    for (const std::string &key : group_keys) {
        if (key.size() != 0){ // 略过空定义    
            auto [_, inserted] = variants_key_map.emplace(key, std::make_tuple(group_index, index));
            if (!inserted) {
                throw RuntimeError("着色器变体键重复定义");
            }
        }
        index++;
    }

    uint32_t group_key_count = group_keys.size();
    uint32_t next_group_base = varient_count;
    variants_key_groups.emplace_back(next_group_base, std::move(group_keys));
    varient_count *= group_key_count;
    assert(varient_count >= group_key_count); // 越界检测
}
void VariantKeyCollect::get_variant_key_names(VariantCode code,
                                                    std::vector<std::string> &out_result) const noexcept {
    for (const auto &[base, group] : variants_key_groups) {
        const std::string &key = group[code / base % (uint32_t)group.size()];
        if (key.size() != 0) { // 空字符串视为不进行定义
            out_result.emplace_back(key);
        }
    }
}
bool VariantKeyCollect::set_varient_code(VariantCode &code, const std::string &varient_key) const noexcept {
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
bool VariantKeyCollect::reset_varient_code(VariantCode &code, const std::string &varient_key) const noexcept {
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
bool VariantKeyCollect::is_key_defined(VariantCode code, const std::string &key) const noexcept {
    if (auto iter = variants_key_map.find(key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        return code / base % (uint32_t)group.size() == index;
    }
    return false;
}


/**
 * @brief 将宏定义等代码插入到着色器中以生成对应变体
 * 插入的代码将替换着色器源码中的#pragma GYA_INJECT
 * @param 着色器代码
 * @param 宏定义
 * @return 生成的代码
 */
 static std::string shader_source_inject(const std::string &src, const std::vector<std::string> &variant_key) {

    const std::string LOACLTING_PATTER = "#pragma GYA_INJECT";

    std::stringstream ss;
    size_t injection_point = src.find(LOACLTING_PATTER);
    if (injection_point == std::string::npos) {
        LOG_WARN("着色器中未找到\"{}\"，无法正确进行变体生成", LOACLTING_PATTER);
    }
    ss << src.substr(0, injection_point);

    ss << "//------Combined Definations---------: \n";

    for (const std::string &key : variant_key) {
        ss << std::format("#ifdef {0}\n#undef {0}\n#endif\n#define {0}\n", key);
    }

    ss << "//------Combined Defination End------: \n";

    ss << src.substr(injection_point + LOACLTING_PATTER.size());

    return ss.str();
}


void ShaderLib::add_uber_shader(const std::string &name, UberShaderDesc &&desc) {
    std::unique_ptr<UberShader> uber_shader{new UberShader()};
    uber_shader->vs_src = std::move(desc.vs_src);
    uber_shader->ps_src = std::move(desc.ps_src);
    uber_shader->global_variant_key_collect = VariantKeyCollect(std::move(desc.global_variant_keys));
    uber_shader->local_variant_key_collect = VariantKeyCollect(std::move(desc.local_variant_keys));
   
    // 立即编译一个不包含任何变体的版本用于反射，此时uber_shader不完整，不能用create_variant
    VariantCodeSet empty{.full_code = 0}; 
    std::vector<std::string> variant_keys;
    uber_shader->get_variant_key_names(empty, variant_keys);
    std::string mixed_vs = shader_source_inject(uber_shader->vs_src, variant_keys);
    std::string mixed_ps = shader_source_inject(uber_shader->ps_src, variant_keys);

    intrusive_ptr<Shader> shader = graphics_api->complie_shader_program(mixed_vs, mixed_ps);
    uber_shader->shaders[empty] = shader; // 既然编译了就加入缓存

    std::unique_ptr<ShaderIntrospector> introspector = graphics_api->create_shader_introspect(shader.get());
    auto buffer_info = introspector->get_constant_buffer_info();

    uber_shader->per_material = std::move(buffer_info["per_material"]);
    uber_shader->per_frame = std::move(buffer_info.at("per_frame"));
    uber_shader->texture_units = introspector->get_texture_info();

    uber_shader->global_key_code = 0; // 记得初始化为0！
    // 设置现有的全局变体键
    for(const std::string& key: global_variant_key_names){
        uber_shader->global_variant_key_collect.set_varient_code(uber_shader->global_key_code, key); 
    }

    uber_shaders.emplace(name, std::move(uber_shader));
}

intrusive_ptr<Shader> UberShader::query_variant(VariantCodeSet variant_code) {
    if (auto iter = shaders.find(variant_code);iter != shaders.end()){
        return iter->second;
    }

    std::vector<std::string> variant_keys;
    get_variant_key_names(variant_code, variant_keys);
    std::string mixed_vs = shader_source_inject(vs_src, variant_keys);
    std::string mixed_ps = shader_source_inject(ps_src, variant_keys);

    intrusive_ptr<Shader> shader = graphics_api->complie_shader_program(mixed_vs, mixed_ps);
    shaders.emplace(variant_code, shader);

    return shader;
}
} // namespace Graphics
} // namespace Goonya