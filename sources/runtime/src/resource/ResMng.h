#pragma once

#include "core/assets.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "resource/scene/scene.h"
#include <filesystem>

namespace Goonya {
// 资源管理器
class RenderResource final {
public:
    Graphics::MeshContainer meshes;
    Graphics::MaterialContainer materials;
    Graphics::Texture2DContainer texture2ds;
    Graphics::TextureCubeMapContainer cubemaps;
    std::unordered_map<AssetKey, Scene::Scene> scenes;

    std::unique_ptr<Graphics::ShaderLib> shader_lib;

    std::unordered_map<AssetKey, Resource::Resource *> resource_cache;

private:
    std::filesystem::path resource_dir;

public:
    void init(const std::filesystem::path& asset_dir) {
        this->resource_dir = asset_dir.lexically_normal();
        shader_lib = std::make_unique<Graphics::ShaderLib>();

        init_buildin_resources();

        scan();
    }

    void clear() {
        meshes.clear();
        materials.clear();
        texture2ds.clear();
        shader_lib.reset();
    }

    AssetKey path_to_key(const std::filesystem::path& path){
        return std::filesystem::relative(path, resource_dir).string();
    }

    void add_shader(const AssetKey &key, Graphics::UberShaderDesc &&desc) const {
        LOG_INFO("Loading Shader: {}", key);
        shader_lib->add_uber_shader(key, std::move(desc));
    }

private:
    void init_buildin_resources();
    void scan();
    void try_load(const std::filesystem::path& path);
};

extern RenderResource resources;
} // namespace Goonya