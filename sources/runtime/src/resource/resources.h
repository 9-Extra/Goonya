#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Goonya {
namespace Resource {

/*
资源键：指向特定资源的一组参数，很多时候它是一个名称。
资源元数据：包括资源键和加载它需要的元数据，比较小，引擎启动后加载所有的资源元数据并永久驻留在内存中。加载资源时，通过元数据加载资源表示。
资源表示：
资源可能以各种形式保存在硬盘上，比如json，比如打包在一起的资源包，甚至可能在运行中生成。加载资源时，先将其加载为统一的资源表示，以方便不同的API进行加载。
+ 包含完整的资源内容，以及其依赖的子资源键（API加载时先检查是不是已经加载了，这是增加引用就行了不需要重新加载）
+ 与API无关（保存在内存中）
+ 可动态创建，可序列化
+ 在加载到设备后即可释放

资源引用：
对已经加载到设备的资源的引用，可以高效复制，一个加载到设备的资源中包含了其对其他的资源的引用，通过引用计数法管理。
+ 设备保存所有已加载资源的资源键及其弱引用，资源引用计数归0时删除记录
+ 可以通过引用反向查找资源键
+ 加载动态创建的资源时如果没有指定资源键，则为其临时创建资源键

加载/使用资源的方法：
1. 资源放进assets文件夹里，元数据写入json中，使用资源键加载
2. 动态创建资源表示，添加依赖的子资源键（包括从资源引用获取），然后调用API加载，注意保持资源引用
3. 动态创建资源键并加载，毕竟资源键可以是参数

*/
struct TextureDesc {
    std::string path;
};

struct ShaderDesc {
    ShaderDesc() noexcept : hash_cache(), uber_name(), definations() {};

    template <typename T1, typename T2>
    ShaderDesc(T1 &&uber_name, T2 &&definations) noexcept : uber_name(uber_name), definations(definations) {
        assert(!uber_name.empty());
        hash_cache = hash();
    }

    ShaderDesc(const ShaderDesc &desc) noexcept
        : hash_cache(desc.hash_cache), uber_name(desc.uber_name), definations(desc.definations) {}
    ShaderDesc(ShaderDesc &&desc) noexcept
        : hash_cache(desc.hash_cache), uber_name(std::move(desc.uber_name)), definations(std::move(desc.definations)) {}

    ShaderDesc &operator=(const ShaderDesc &desc) noexcept = default;

    const std::string &get_uber_name() const noexcept { return uber_name; }
    const std::unordered_map<std::string, std::string> &get_definations() const noexcept { return definations; }

    bool operator==(const ShaderDesc &b) const noexcept {
        return hash_cache == b.hash_cache && uber_name == b.uber_name && definations == b.definations;
    }

private:
    friend struct std::hash<Goonya::Resource::ShaderDesc>;
    size_t hash_cache;
    std::string uber_name;
    std::unordered_map<std::string, std::string> definations;

    size_t hash() const noexcept {
        size_t result = std::hash<std::string>{}(uber_name);
        for (const auto &[k, v] : definations) {
            result ^= std::hash<std::string>{}(k) ^ std::hash<std::string>{}(v);
        }
        return result;
    }
};

struct PSODesc {
    ShaderDesc shader_desc;

    bool enable_cilp;                 // glEnable(GL_CULL_FACE)
    std::string cull_face_mode;       // glCullFace
    std::string front_face_clockwise; // glFrontFace

    bool enable_depth_test; // glEnable(GL_DEPTH_TEST)
    std::string depth_func; // glDepthFunc

    bool operator==(const PSODesc &b) const noexcept = default;
};

struct MaterialDesc {
    struct UniformDataDesc {
        const uint32_t binding_id;
        const uint32_t size;
        const void *data = nullptr;
    };

    struct SamplerData {
        uint32_t binding_id;
        std::string texture_key;
        std::string texture_type;
        std::string warp_mode;
        std::string filter_mode;
    };

    PSODesc pso_desc;
    std::vector<UniformDataDesc> uniforms;
    std::vector<SamplerData> samplers;
};

} // namespace Resource
} // namespace Goonya

template <>
struct std::hash<Goonya::Resource::ShaderDesc> {
    size_t operator()(const Goonya::Resource::ShaderDesc &desc) const noexcept { return desc.hash_cache; }
};

template <>
struct std::hash<Goonya::Resource::PSODesc> {
    size_t operator()(const Goonya::Resource::PSODesc &desc) const noexcept {
        return std::hash<decltype(desc.shader_desc)>{}(desc.shader_desc) &
               std::hash<decltype(desc.cull_face_mode)>{}(desc.cull_face_mode) &
               std::hash<decltype(desc.front_face_clockwise)>{}(desc.front_face_clockwise) &
               std::hash<decltype(desc.depth_func)>{}(desc.depth_func) & desc.enable_depth_test << 3 & desc.enable_cilp;
    }
};