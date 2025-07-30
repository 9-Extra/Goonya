#pragma once

#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "resource/scene/scene.h"

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
} // namespace Goonya