#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

#include "core/intrusive_ptr.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/graphics.h"
#include "core/log/Log.h"

namespace Goonya {
namespace Graphics {

struct Texture2DDesc {
    std::filesystem::path path;
    bool is_srgb = false;         // 是否需要转换到线性空间
    bool is_uv_left_down = false; // UV坐标系是否以左下角为原点
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

struct TextureCubeMapDesc {
    std::array<std::filesystem::path, 6> path; // px, nx, py, ny, pz, nz
    bool is_srgb = false;            // 是否需要转换到线性空间
    bool is_uv_left_down = false;    // UV坐标系是否以左下角为原点
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

// 资源管理器
class RenderReousce final {
public:
    template <class T>
    using ResourceContainer = std::unordered_map<std::string, intrusive_ptr<T>>;

    ResourceContainer<Mesh> meshes;
    ResourceContainer<Material> materials;
    ResourceContainer<Texture> textures;

    std::unique_ptr<ShaderLib> shader_lib;

    void init(){
        shader_lib = std::make_unique<ShaderLib>();
    }

    void clear() {
        meshes.clear();
        materials.clear();
        textures.clear();
        shader_lib.reset();
    }

    void add_mesh(const std::string &key, const Graphics::VertexLayout &vertex_layout,
                  std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices,
                  Topology topology = Topology::TRIANGLE) {
        LOG_INFO("Loading Mesh: {}", key);
        meshes.emplace(key, graphics_api->load_mesh(topology, vertex_layout, raw_vertices, indices));
    };
    template <typename D>
        requires std::is_trivially_copyable_v<D> && (!std::is_same_v<D, uint8_t>)
    void add_mesh(const std::string &key, const Graphics::VertexLayout &vertex_layout, std::span<const D> vertices,
                  std::span<const uint16_t> indices, Topology topology = Topology::TRIANGLE) {
        add_mesh(key, vertex_layout, std::span((uint8_t *const)vertices.data(), vertices.size_bytes()), indices,
                 topology);
    }

    template <typename D>
        requires std::is_trivially_copyable_v<D> && (!std::is_same_v<D, uint8_t>)
    void add_mesh(const std::string &key, const Graphics::VertexLayout &vertex_layout, std::span<D> vertices,
                  std::span<const uint16_t> indices, Topology topology = Topology::TRIANGLE) {
        add_mesh(key, vertex_layout, std::span((uint8_t *const)vertices.data(), vertices.size_bytes()), indices,
                 topology);
    }

    void add_material(const std::string &key, const MaterialDesc &desc) {
        LOG_INFO("Loading Material: {}", key);
        intrusive_ptr<Material> mat{graphics_api->create_material(shader_lib->query_uber_shader(desc.uber_shader_name))};
        mat->set_depth_test_mode(desc.depth_test);
        mat->set_cull_mode(desc.cull_mode);
        
        for(const auto& [name, value]: desc.parameters){
            mat->set_param(name, value);
        }
        for(const auto& [name, texture_key]: desc.textures){
            mat->set_texture(name, textures.at(texture_key));
        }
        materials.emplace(key, mat);
    }
    void add_texture2d(const std::string &key, const Texture2DDesc &desc);
    void add_cubemap(const std::string &key, const TextureCubeMapDesc &desc);
    void add_shader(const std::string &key, UberShaderDesc&& desc) {
        LOG_INFO("Loading Shader: {}", key);
        shader_lib->add_uber_shader(key, std::move(desc));
    }
};

extern RenderReousce resources;

} // namespace Graphics
} // namespace Goonya