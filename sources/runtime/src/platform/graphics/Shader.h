#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "runtime/GoonyaException.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Goonya {
namespace Graphics {

struct UberShaderDesc {
    std::string vs_src;
    std::string ps_src;

    /**
     * 当全局的变体定义发生修改时，所有使用此定义的UberShader的所有材质受到影响并更新，
     * 而局部的变体定义只能在对应的材质上进行更改，不会影响其他材质，
     * 变体不能影响材质参数对应的UniformBuffer的参数和内存布局，一个UberShader下所有的变体使用完全相同的布局
     * 单个UberShader中全局和局部的关键字定义不可以相同
     *
     * 每个外层vector是一个组，内层是组内的变体（每组中有且只有一个变体启用）
     */
    std::vector<std::vector<std::string>> global_variant_keys; // 相关的变体定义
    std::vector<std::vector<std::string>> local_variant_keys;  // 仅此UberShader使用的所有变体定义
};

using VariantCode = uint32_t;

union VariantCodeSet {
    struct {
        VariantCode global_code;
        VariantCode loacl_code;
    };
    uint64_t full_code;

    bool operator==(const VariantCodeSet other) const noexcept{
        return this->full_code == other.full_code;
    }
};
// 检测VariantCodeSet以希望的方式对齐
static_assert(offsetof(VariantCodeSet, global_code) == 0 && offsetof(VariantCodeSet, loacl_code) == 4);

} // namespace Graphics
} // namespace Goonya

template <>
struct std::hash<Goonya::Graphics::VariantCodeSet> {
    size_t operator()(Goonya::Graphics::VariantCodeSet code) const noexcept { return code.full_code; }
};

namespace Goonya {
namespace Graphics {

class ShaderVariantKeyCollect final{
public:
    ShaderVariantKeyCollect() {}
    ShaderVariantKeyCollect(std::vector<std::vector<std::string>> &&group_keys) {
        for (std::vector<std::string> &g : group_keys) {
            add_variant_key_group(std::move(g));
        }
    }

    uint32_t get_varient_count() const noexcept{
        return varient_count;
    }

    void add_variant_key_group(std::vector<std::string> &&group_keys);

    void get_variant_key_names(VariantCode code, std::vector<std::string> &out_result) const noexcept;

    bool set_varient_code(VariantCode& code, const std::string &varient_key) const noexcept;
    /**
     * @brief 设置指定varient_key所在组为默认
     * 不存在“移除”一个变体键的方法，因为那样产生的变体可能是不合法的，最多重置成默认情况
     * @param code 码
     * @param varient_key 要“移除”的键，同组中任意一个键效果相同
     * @return 是否发生更新
     */
    bool reset_varient_code(VariantCode& code, const std::string &varient_key) const noexcept;
    bool is_key_defined(VariantCode code, const std::string &key) const noexcept;

private:
    // 着色器组及其组的基
    std::vector<std::tuple<uint32_t, std::vector<std::string>>> variants_key_groups;
    // 变体所在组号及其在组中的序号
    std::unordered_map<std::string, std::tuple<uint8_t, uint8_t>> variants_key_map;
    uint32_t varient_count = 1;
};

struct ShaderDesc {
    std::string uber_name;
    std::vector<std::string> variant_keys;

    ShaderDesc() {}

    template <typename T1, typename T2>
    ShaderDesc(T1 &&uber_name, T2 &&variant_code) noexcept
        : uber_name(std::forward<T1>(uber_name)), variant_keys(std::forward<T2>(variant_keys)) {
        assert(!uber_name.empty());
    }

    ShaderDesc(const ShaderDesc &desc) noexcept : uber_name(desc.uber_name), variant_keys(desc.variant_keys) {}
    ShaderDesc(ShaderDesc &&desc) noexcept
        : uber_name(std::move(desc.uber_name)), variant_keys(std::move(desc.variant_keys)) {}

    ShaderDesc &operator=(const ShaderDesc &desc) noexcept = default;
    bool operator==(const ShaderDesc &b) const noexcept = default;
};

enum class CullFaceMode{
    BACK = 0,
    FRONT,
    FRONT_AND_BACK,
    DISABLE,
};

enum class DepthTestMode{
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    NEVER,
    ALWAYS,
    DISABLE
};

struct Vertex {
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f uv;
};

struct ShaderUniformBlockInfo {
    Meta::LayoutInfo layout;
    uint32_t binding;
};

class Shader;
struct UberShader {
    std::string vs_src;
    std::string ps_src;

    ShaderVariantKeyCollect global_variant_key_collect; // 全局变体编码器
    ShaderVariantKeyCollect local_variant_key_collect;

    VariantCode global_key_code; // 当前元着色器的全局变体定义编码

    std::unordered_map<VariantCodeSet, intrusive_ptr<Shader>> shaders; // 此元着色器的变体缓存

    // UniformBuffer内存布局
    ShaderUniformBlockInfo per_material;
    ShaderUniformBlockInfo per_frame;
    std::unordered_map<std::string, uint32_t> texture_units; // 纹理名称及对应的纹理单元
};

class Shader : public intrusive_ptr_base<Shader> {
public:
    virtual void bind() = 0;
    virtual ~Shader() = default;

    const UberShader *get_uber_shader() { return uber_shader; }
    VariantCodeSet get_variant_code() { return variant_code; }

protected:
    Shader(const UberShader* uber_shader, VariantCodeSet code): uber_shader(uber_shader), variant_code(code) {}
    const UberShader *uber_shader;
    const VariantCodeSet variant_code;
};

class ShaderLib {
public:
    virtual ~ShaderLib() = default;

    virtual void add_uber_shader(const std::string &name, UberShaderDesc &&desc) = 0;

    bool is_global_varient_key_set(const std::string& key) const noexcept{
        return global_variant_key_names.contains(key);
    }
    void set_global_varient_key(const std::string& key) noexcept{
        auto [_, is_update] = global_variant_key_names.emplace(key);
        if (is_update){
            for(auto& [name, uber]: uber_shaders){
                uber->global_variant_key_collect.set_varient_code(uber->global_key_code, key);
            }
        }
    }

    void reset_global_varient_key(const std::string& key) noexcept{
        if(auto iter = global_variant_key_names.find(key);iter != global_variant_key_names.end()){
            global_variant_key_names.erase(key);
            for(auto& [name, uber]: uber_shaders){
                uber->global_variant_key_collect.reset_varient_code(uber->global_key_code, key);
            }
        }
    }

    std::vector<std::string> decode_varient_keys(UberShader *uber_shader, VariantCodeSet variant_code) {
        std::vector<std::string> result;
        uber_shader->global_variant_key_collect.get_variant_key_names(variant_code.global_code, result);
        uber_shader->local_variant_key_collect.get_variant_key_names(variant_code.loacl_code, result);
        return result;
    }

    UberShader* query_uber_shader(const std::string& uber_shader_name) {
        if (!uber_shaders.contains(uber_shader_name)){
            throw RuntimeError(std::format("元着色器\"{}\"未注册", uber_shader_name));
        }
        return uber_shaders.at(uber_shader_name).get();
    }

    intrusive_ptr<Shader> query_shader(UberShader *uber_shader, VariantCodeSet variant_code) {
        if (auto iter = uber_shader->shaders.find(variant_code); iter != uber_shader->shaders.end()) {
            return iter->second;
        }

        intrusive_ptr<Shader> shader = create_variant(uber_shader, variant_code);
        uber_shader->shaders.emplace(variant_code, shader);

        return shader;
    }

protected:
    virtual intrusive_ptr<Shader> create_variant(UberShader *uber_shader, VariantCodeSet variant_code) = 0;
   
    std::unordered_set<std::string> global_variant_key_names; // 全局着色器变体定义

    std::unordered_map<std::string, std::unique_ptr<UberShader>> uber_shaders; // 从名称到UberShader
   
    // 从全局定义到此定义影响的UberShader
    // std::unordered_map<std::string, UberShader *> global_key_relat_uber_shader;
};

} // namespace Graphics
} // namespace Goonya
