#pragma once

#include <array>
#include <cassert>
#include <filesystem>
#include <json/config.h>
#include <json/json.h>
#include <json/value.h>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "core/assets.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include "resource/scene/scene.h"
#include "runtime/GoonyaException.h"

namespace Goonya::Resource {

class Resource;

struct ResourceClassInfo {
    Resource *(*creator)(); // 无参构造函数，调用后返回Resource*
};

// 抄Godot的ClassDB，但现在只管Resource
// 所有成员都是静态的
class ResourceClassDB final {
private:
    static std::unordered_map<std::string, ResourceClassInfo> class_infos;

public:
    template <class T>
    static void register_class() noexcept {
        class_infos.emplace(T::get_class_name_static(), ResourceClassInfo{[]() { return T::T(); }});
    }

    static bool is_class_registered(const std::string& class_name) noexcept{
        return class_infos.contains(class_name);
    }

    static const std::unordered_map<std::string, ResourceClassInfo>& get_class_info() noexcept {
        return class_infos;
    }
};

#define DEFINE_CLASS(class_name)                                                                                       \
    friend class ::Goonya::Resource::ResourceClassDB;                                                                  \
    static const std::string_view get_class_name_static() noexcept { return #class_name; }                                      \
    virtual std::string_view get_class_name() const noexcept { return #class_name; }                                                  \
                                                                                                                       \
private:

// 所有资源继承此类
class Resource : public intrusive_ptr_base<Resource> {
    DEFINE_CLASS(Resource);

private:
    std::string name; // 单纯为了看，没有查找等用途

public:
    Resource() = default;
    virtual ~Resource() = default;

    const std::string &get_name() const noexcept { return name; }
    void set_name(std::string name) noexcept { this->name = std::move(name); }

    /**
     * @brief 将自身需要记录的字段写入json对象
     * 可能需要从显卡读出数据再写入json
     * 待写入的json对象是一个以及创建好的json object对象(json.type() == Json::ValueType::objectValue),
     * 将json对象作为一种资源的中间表示，以实现资源之间的嵌套及资源序列化
     * 但这种每类资源各自实现自身序列化的设计在处理资源之间的继承关系时不够自然，子类必须显式调用父类的save_to_json以序列化父类字段
     */
    virtual void save_property_to_json(Json::Value &json) const {
        if (!name.empty()) {
            json["name"] = name;
        }
    }
    /**
     * @brief 从json恢复自身数据
     * 可能需要读出数据后上传显卡
     * 记得检查并加载子资源
     * @param json
     */
    virtual void load_property_from_json(const Json::Value &json) {
        name = json.get("name", get_class_name().data()).asString();
    };

    /**
     * @brief 将此资源保存到json
     * 加载单个资源的流程是：
     *   + Json库从文件加载json
     *  + 从ResourceClassDB找到json中类名对应的构造函数进行无参构造，得到C++资源对象Resource*
     * + 利用C++的动态绑定，直接调用res->load_from_json()就可以正确加载资源
     * json格式为
     * {
     *     "class_name": "资源对象C++类名"
     *     "properties": {
     *         ... // 由资源@ref save_property_to_json()函数写入的部分
     *         // 子资源也应该使用同样的加载机制，无论其类型是否已知，可以在加载完后使用dynamic_cast检查类型是否符合预期
     *         // 子资源的保存同样由对应类的@ref save_property_to_json实现
     *         "sub_resource": {
     *             "class_name": "子资源的C++类名",
     *             "properties": {...}
     *         }
     *     }
     * }
     *
     * @note 一般不需要重载这个函数，应该去重载@ref load_property_from_json
     * @note 目前只实现序列化为文本（json）
     * @return Json::Value
     */
    static Json::Value save_to_json(intrusive_ptr<Resource> res) {
        assert(res);
        Json::Value json{Json::objectValue};
        json["class_name"] = res->get_class_name().data();
        json["properties"] = Json::Value{Json::objectValue};
        res->save_property_to_json(json["properties"]);
        return json;
    }

    /**
     * @brief 从保存的Json加载资源
     * 
     */
    static intrusive_ptr<Resource> load_from_json(const Json::Value& json) {
        const std::unordered_map<std::string, ResourceClassInfo>& infos = ResourceClassDB::get_class_info();
        const std::string& class_name = json["class_name"].asString();
        if (auto iter = infos.find(class_name);iter != infos.end()){
            const ResourceClassInfo& info = iter->second;

            intrusive_ptr<Resource> res = std::invoke(info.creator); // 调用对应类型的构造函数
            res->load_property_from_json(json["properties"]);
            
            return res;
        } else {    
            LOG_ERROR("加载时，资源类型 {} 未注册或者缺失", class_name);
            return nullptr;
        }
    }
};

struct Texture2DDesc {
    std::filesystem::path path;
    bool is_color;
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

struct TextureCubeMapDesc {
    std::array<std::filesystem::path, 6> path;
    bool is_color = false;
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

template <class TDesc, class TAsset>
class ResourceContainer {
protected:
    std::string name;
    std::unordered_map<AssetKey, std::tuple<intrusive_ptr<TAsset>, TDesc>> container;

public:
    virtual ~ResourceContainer() = default;
    explicit ResourceContainer(std::string name) : name(std::move(name)) {}
    template <typename T>
        requires std::is_convertible_v<T, TDesc>
    void add(const AssetKey &key, T &&desc) {
        auto [_, ok] = container.emplace(key, std::tuple<intrusive_ptr<TAsset>, TDesc>{nullptr, std::forward<T>(desc)});
        if (!ok) {
            throw RuntimeError(std::format("资源\"{}\"重复注册", key));
        }
    }
    bool contains(const AssetKey &key) const { return container.contains(key); }

    intrusive_ptr<TAsset> get(const AssetKey &key) {
        if (auto iter = container.find(key); iter != container.end()) {
            auto &[asset, desc] = iter->second;
            if (!asset) [[unlikely]] {
                LOG_TRACE("正在加载{}：\"{}\"", name, key);
                asset = load(desc);
            }
            return asset;
        } else {
            throw RuntimeError(std::format("资源\"{}\"未注册", key));
        }
    }

    void clear() { container.clear(); }

protected:
    virtual intrusive_ptr<TAsset> load(const TDesc &desc) const = 0;
};

class MeshContainer final : public ResourceContainer<Graphics::MeshDesc, Graphics::Mesh> {
public:
    MeshContainer() : ResourceContainer<Graphics::MeshDesc, Graphics::Mesh>("网格") {}

protected:
    intrusive_ptr<Graphics::Mesh> load(const Graphics::MeshDesc &desc) const override;
};

class MaterialContainer final : public ResourceContainer<Graphics::MaterialDesc, Graphics::Material> {
public:
    MaterialContainer() : ResourceContainer<Graphics::MaterialDesc, Graphics::Material>("材质") {}

protected:
    intrusive_ptr<Graphics::Material> load(const Graphics::MaterialDesc &desc) const override;
};

class Texture2DContainer final : public ResourceContainer<Texture2DDesc, Graphics::Texture> {
public:
    Texture2DContainer() : ResourceContainer<Texture2DDesc, Graphics::Texture>("纹理") {}

protected:
    intrusive_ptr<Graphics::Texture> load(const Texture2DDesc &desc) const override;
};

class TextureCubeMapContainer final : public ResourceContainer<TextureCubeMapDesc, Graphics::Texture> {
public:
    TextureCubeMapContainer() : ResourceContainer<TextureCubeMapDesc, Graphics::Texture>("纹理") {}

protected:
    intrusive_ptr<Graphics::Texture> load(const TextureCubeMapDesc &desc) const override;
};

// 资源管理器
class RenderResource final {
public:
    MeshContainer meshes;
    MaterialContainer materials;
    Texture2DContainer texture2ds;
    TextureCubeMapContainer cubemaps;
    std::unordered_map<AssetKey, Scene::Scene> scenes;

    std::unique_ptr<Graphics::ShaderLib> shader_lib;

public:
    void init() { shader_lib = std::make_unique<Graphics::ShaderLib>(); }

    void clear() {
        meshes.clear();
        materials.clear();
        texture2ds.clear();
        shader_lib.reset();
    }

    void add_shader(const AssetKey &key, Graphics::UberShaderDesc &&desc) const {
        LOG_INFO("Loading Shader: {}", key);
        shader_lib->add_uber_shader(key, std::move(desc));
    }
};

extern RenderResource resources;

} // namespace Goonya::Resource
