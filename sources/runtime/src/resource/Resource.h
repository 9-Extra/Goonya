#pragma once

#include <cassert>
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
#include "platform/graphics/UberShader.h"
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

    static bool is_class_registered(const std::string &class_name) noexcept { return class_infos.contains(class_name); }

    static const std::unordered_map<std::string, ResourceClassInfo> &get_class_info() noexcept { return class_infos; }
};

#define DEFINE_CLASS(class_name, desc_type)                                                                            \
    friend class ::Goonya::Resource::ResourceClassDB;                                                                  \
    static const std::string_view get_class_name_static() noexcept { return #class_name; }                             \
    virtual std::string_view get_class_name() const noexcept { return #class_name; }                                   \
    using DescType = desc_type;                                                                                                             \
private:

// 所有资源继承此类
class Resource : public intrusive_ptr_base<Resource> {
    DEFINE_CLASS(Resource, int);

private:
    std::string name; // 单纯为了看，没有查找等用途

public:
    Resource() = default;
    virtual ~Resource() = default;

    const std::string &get_name() const noexcept { return name; }
    void set_name(std::string name) noexcept { this->name = std::move(name); }


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

class Texture2DContainer final : public ResourceContainer<Graphics::Texture2DDesc, Graphics::Texture> {
public:
    Texture2DContainer() : ResourceContainer<Graphics::Texture2DDesc, Graphics::Texture>("纹理") {}

protected:
    intrusive_ptr<Graphics::Texture> load(const Graphics::Texture2DDesc &desc) const override;
};

class TextureCubeMapContainer final : public ResourceContainer<Graphics::TextureCubeMapDesc, Graphics::Texture> {
public:
    TextureCubeMapContainer() : ResourceContainer<Graphics::TextureCubeMapDesc, Graphics::Texture>("纹理") {}

protected:
    intrusive_ptr<Graphics::Texture> load(const Graphics::TextureCubeMapDesc &desc) const override;
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

    std::unordered_map<AssetKey, Resource *> resource_cache;

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
