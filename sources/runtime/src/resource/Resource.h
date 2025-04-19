#pragma once

#include <array>
#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "core/asserts.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include "runtime/GoonyaException.h"

namespace Goonya::Resource {

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

    std::string name;
    std::unordered_map<AssetKey, std::tuple<intrusive_ptr<TAsset>, TDesc>> container;
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

    std::unique_ptr<Graphics::ShaderLib> shader_lib;

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
