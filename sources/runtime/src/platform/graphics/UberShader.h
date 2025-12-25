#pragma once

#include "core/RefCount.h"
#include "core/hash_helper.h"
#include "core/log/Log.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/Resource.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Goonya {

struct UberShaderDesc final {
    std::string vs_src;
    std::string ps_src;

    PipelineSetting pipeline_setting; // 着色器默认的渲染管线设置

    /**
     * 当全局的变体定义发生修改时，所有使用此定义的UberShader的所有材质受到影响并更新，
     * 而局部的变体定义只能在对应的材质上进行更改，不会影响其他材质，
     * 变体不能影响材质参数对应的UniformBuffer的参数和内存布局，一个UberShader下所有的变体使用完全相同的布局
     * 单个UberShader中全局和局部的关键字定义不可以相同
     *
     * 每个外层vector是一个组，内层是组内的变体（每组中有且只有一个变体启用）
     */
    std::vector<std::string> global_variant_keys;             // 相关的变体定义
    std::vector<std::vector<std::string>> local_variant_keys; // 仅此UberShader使用的所有变体定义
};

using VariantCode = uint32_t;

union VariantCodeSet {
    struct {
        VariantCode global_code;
        VariantCode local_code;
    };
    uint64_t full_code;

    explicit VariantCodeSet(VariantCode global_code, VariantCode local_code)
        : global_code(global_code), local_code(local_code) {}
    explicit VariantCodeSet(uint64_t full_code) : full_code(full_code) {}

    bool operator==(const VariantCodeSet other) const noexcept { return this->full_code == other.full_code; }
};
// 检测VariantCodeSet以希望的方式对齐
static_assert(offsetof(VariantCodeSet, global_code) == 0 && offsetof(VariantCodeSet, local_code) == 4);

} // namespace Goonya

template <>
struct std::hash<Goonya::VariantCodeSet> {
    size_t operator()(Goonya::VariantCodeSet code) const noexcept { return code.full_code; }
};

namespace Goonya {

class LocalVariantKeyCollect final {
private:
    // 着色器组及其组的基
    std::vector<std::tuple<uint32_t, std::vector<std::string>>> variants_key_groups;
    // 变体所在组号及其在组中的序号
    std::unordered_map<std::string, std::tuple<uint8_t, uint8_t>, StringHash, StringEqual> variants_key_map;
    uint32_t variant_count = 1;

public:
    LocalVariantKeyCollect() = default;
    explicit LocalVariantKeyCollect(std::vector<std::vector<std::string>> &&group_keys) {
        for (std::vector<std::string> &g : group_keys) {
            add_variant_key_group(std::move(g));
        }
    }

    uint32_t get_variant_count() const noexcept { return variant_count; }

    void add_variant_key_group(std::vector<std::string> &&group_keys);

    void get_variant_key_names(VariantCode code, std::vector<std::string> &out_result) const noexcept;

    bool set_variant_code(VariantCode &code, std::string_view variant_key) const noexcept;
    /**
     * @brief 设置指定varient_key所在组为默认
     * 不存在“移除”一个变体键的方法，因为那样产生的变体可能是不合法的，最多重置成默认情况
     * @param code 码
     * @param variant_key 要“移除”的键，同组中任意一个键效果相同
     * @return 是否发生更新
     */
    bool reset_variant_code(VariantCode &code, std::string_view variant_key) const noexcept;
    bool is_key_defined(VariantCode code, std::string_view variant_key) const noexcept;
};

class GlobalVariantKeyCollect final {
public:
    const static uint32_t MAX_GLOVAL_KEY_COUNR = std::numeric_limits<VariantCode>::digits;
    static_assert(MAX_GLOVAL_KEY_COUNR < std::numeric_limits<uint16_t>::max());

protected:
    std::bitset<MAX_GLOVAL_KEY_COUNR> key_mask;
    std::array<std::string, MAX_GLOVAL_KEY_COUNR> id_to_key;
    std::unordered_map<std::string, uint16_t, StringHash, StringEqual> key_to_id; // 全局着色器变体定义
public:
    bool is_key_set(std::string_view key) const noexcept {
        if (auto iter = key_to_id.find(key); iter != key_to_id.end()) {
            return key_mask[iter->second];
        } else {
            return false;
        }
    }

    void get_variant_key_names(std::vector<std::string> &out_result) const noexcept {
        uint16_t size = key_to_id.size();
        for (uint16_t i = 0; i < size; i++) {
            if (key_mask[i]) {
                out_result.emplace_back(id_to_key[i]);
            }
        }
    }

    void set_key(std::string_view key) noexcept {
        uint16_t id = register_key(key);
        if (id == MAX_GLOVAL_KEY_COUNR) {
            return;
        }
        if (!key_mask[id]) {
            key_mask[id] = true;
        }
    }

    void reset_key(std::string_view key) noexcept {
        uint16_t id = register_key(key);
        if (id == MAX_GLOVAL_KEY_COUNR) {
            return;
        }
        if (key_mask[id]) {
            key_mask[id] = false;
        }
    }

    VariantCode get_global_code() noexcept {
        // 不要直接使用此结果作为global_code，还需要与UberShader中的effective_global_key_mask进行掩码操作
        return (VariantCode)key_mask.to_ullong();
    }

    VariantCode get_shader_global_key_mask(const std::vector<std::string> &global_keys) noexcept {
        std::bitset<MAX_GLOVAL_KEY_COUNR> mask;
        for (const std::string &key : global_keys) {
            uint16_t id = register_key(key);
            if (id == MAX_GLOVAL_KEY_COUNR) {
                continue;
            }
            mask[id] = true;
        }
        return (VariantCode)mask.to_ullong();
    }

private:
    uint16_t register_key(std::string_view key) noexcept {
        if (auto iter = key_to_id.find(key); iter != key_to_id.end()) {
            return iter->second;
        }

        if (key.empty()) {
            LOG_ERROR("全局着色器变体名不应该为空，因为其本身设计就是要么空值，要么有值的二元选择");
        }
        uint16_t id = (uint16_t)key_to_id.size();
        if (id >= MAX_GLOVAL_KEY_COUNR) {
            LOG_ERROR("全局着色器变体超过上限");
            return MAX_GLOVAL_KEY_COUNR;
        }

        key_to_id.emplace(key, id);
        id_to_key[id] = key;

        return id;
    }
};

extern GlobalVariantKeyCollect GLOBAL_VARIANT_KEY;

class GLShader;
class UberShader final : public Resource {
protected:
    // 创建后会变的
    std::unordered_map<VariantCodeSet, Ref<GLShader>> shaders; // 此元着色器的变体缓存
    // 不会变的
    std::string vs_src;
    std::string ps_src;

    // 默认材质参数
    PipelineSetting pipeline_setting;                              // 着色器默认的渲染管线设置
    std::unordered_map<std::string, Meta::DynamicData> parameters; // 默认材质参数
    std::unordered_map<uint32_t, Ref<GLTexture>> textures;         // 默认纹理绑定

    LocalVariantKeyCollect local_variant_key_collect;
    VariantCode effective_global_key_mask;

    // 几个特殊的UniformBuffer内存布局
    ShaderUniformBlockInfo per_material;
    ShaderUniformBlockInfo per_frame;
    ShaderUniformBlockInfo per_object;
    std::unordered_map<std::string, ShaderUniformBlockInfo> uniform_info; // 全部uniform的内存布局和绑定点
    std::unordered_map<std::string, uint32_t> texture_units;              // 纹理名称及对应的纹理单元
public:
    explicit UberShader(UberShaderDesc &&desc);
    UberShader(const UberShader &) = delete;
    UberShader(UberShader &&) = delete;

    const PipelineSetting &get_pipeline_setting() const noexcept { return pipeline_setting; }

    const ShaderUniformBlockInfo &per_material_block() const noexcept { return per_material; }
    const ShaderUniformBlockInfo &per_frame_block() const noexcept { return per_frame; }
    const ShaderUniformBlockInfo &per_object_block() const noexcept { return per_object; }
    const std::unordered_map<std::string, uint32_t> &get_texture_units() const noexcept { return texture_units; }
    const std::unordered_map<std::string, ShaderUniformBlockInfo> &get_uniform_info() const noexcept {
        return uniform_info;
    }

    VariantCode get_effective_global_key_code() const noexcept {
        return GLOBAL_VARIANT_KEY.get_global_code() & effective_global_key_mask;
    }
    Ref<GLShader> query_variant(VariantCodeSet variant_code);

    void get_variant_key_names(VariantCodeSet code, std::vector<std::string> &out_names) const noexcept {
        local_variant_key_collect.get_variant_key_names(code.local_code, out_names);
        GLOBAL_VARIANT_KEY.get_variant_key_names(out_names);
    }

    bool set_local_variant_key(VariantCode local_code, const std::string &variant_key) const noexcept {
        return local_variant_key_collect.set_variant_code(local_code, variant_key);
    }
    bool reset_local_variant_key(VariantCode local_code, const std::string &variant_key) const noexcept {
        return local_variant_key_collect.reset_variant_code(local_code, variant_key);
    }
};

struct GlobalKeyChangeEvent {
    std::vector<std::string> key_set_current_frame;
    std::vector<std::string> key_reset_current_frame;
};

} // namespace Goonya
