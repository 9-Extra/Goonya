#pragma once

#include "core/metatype/metatype.h"
#include "core/hash_helper.h"
#include "platform/graphics/Texture.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>

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

struct Texture2DDesc {
    std::string path;
    bool is_srgb = false; // 是否需要转换到线性空间
    bool is_uv_left_down = false; // UV坐标系是否以左下角为原点
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

struct TextureCubeMapDesc {
    std::array<std::string, 6> path; // px, nx, py, ny, pz, nz
    bool is_srgb = false; // 是否需要转换到线性空间
    bool is_uv_left_down = false; // UV坐标系是否以左下角为原点
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

struct ShaderDesc {
    std::string uber_name;
    std::unordered_set<std::string> variant_keys;

    ShaderDesc() {}

    template <typename T1, typename T2>
    ShaderDesc(T1 &&uber_name, T2 &&variant_keys) noexcept : uber_name(uber_name), variant_keys(variant_keys) {
        assert(!uber_name.empty());
    }

    ShaderDesc(const ShaderDesc &desc) noexcept
        : uber_name(desc.uber_name), variant_keys(desc.variant_keys) {}
    ShaderDesc(ShaderDesc &&desc) noexcept
        : uber_name(std::move(desc.uber_name)), variant_keys(std::move(desc.variant_keys)) {}

    ShaderDesc &operator=(const ShaderDesc &desc) noexcept = default;
    bool operator==(const ShaderDesc &b) const noexcept = default;


private:
    friend struct std::hash<Goonya::Resource::ShaderDesc>;
    size_t hash() const noexcept {
        size_t seed = std::hash<std::string>{}(uber_name);
        for (const std::string& key : variant_keys) {
            hash_combine(seed, key);
        }
        return seed;
    }
};


/*
定义着色器编写规范：每一个Location绑定的数据都是指定的。可少不可多，名字可以改，类型不能变
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec3 tangent;
    layout (location = 3) in vec2 uv;

如果着色器需要的数据Mesh中没有，UB。
*/

enum class VertexAttribute: uint32_t{
    POSITION = 0, // 指定location
    NORMAL = 1,
    TANGENT = 2,
    UV = 3,
};

struct VertexLayout{
    // 用途，类型，偏移量
    std::vector<std::tuple<VertexAttribute, Meta::FieldType, size_t>> attributes;
    size_t size; // 单个顶点大小

    bool operator==(const VertexLayout &b) const noexcept = default;
    size_t hash() const noexcept{
        size_t seed = size;
        for(const auto& t : attributes){
            hash_combine(seed, hash_tuple(t));
        }
        return seed;
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
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::vector<std::tuple<std::string, std::string>> textures; // (着色器中名称, 资源键)

    PSODesc pso_desc;
};

} // namespace Resource
} // namespace Goonya

template <>
struct std::hash<Goonya::Resource::ShaderDesc> {
    size_t operator()(const Goonya::Resource::ShaderDesc &desc) const noexcept { return desc.hash(); }
};

template <>
struct std::hash<Goonya::Resource::PSODesc> {
    size_t operator()(const Goonya::Resource::PSODesc &desc) const noexcept {
        size_t seed = 0;
        Goonya::hash_combine(seed, desc.shader_desc);
        Goonya::hash_combine(seed, desc.cull_face_mode);
        Goonya::hash_combine(seed, desc.front_face_clockwise);
        Goonya::hash_combine(seed, desc.depth_func);
        return seed ^ desc.enable_depth_test << 3 ^ desc.enable_cilp;
    }
};