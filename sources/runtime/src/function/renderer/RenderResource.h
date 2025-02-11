#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform/graphics/GraphicsResource.h"
#include "platform/graphics/graphics.h"
#include "runtime/log/Log.h"

namespace Goonya {
namespace Graphics {

// 资源管理器
class RenderReousce final {
public:
    template <class T>
    using ResourceContainer = std::unordered_map<std::string, intrusive_ptr<T>>;

    ResourceContainer<Mesh> meshes;
    ResourceContainer<Material> materials;
    ResourceContainer<Texture> textures;

    void clear(){
        meshes.clear();
        materials.clear();
        textures.clear();
    }

    void add_mesh(const std::string &key, const Resource::VertexLayout& vertex_layout, std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices, Topology topology = Topology::TRIANGLE) {
        LOG_INFO("Loading Mesh: {}", key);
        meshes.emplace(key, graphics_api->load_mesh(topology, vertex_layout, raw_vertices, indices));
    };
    template<typename D> requires std::is_trivially_copyable_v<D> && (!std::is_same_v<D, uint8_t>)
    void add_mesh(const std::string &key, const Resource::VertexLayout& vertex_layout, std::span<const D> vertices, std::span<const uint16_t> indices, Topology topology = Topology::TRIANGLE){
        add_mesh(key, vertex_layout, std::span((uint8_t* const)vertices.data(), vertices.size_bytes()), indices, topology);
    }

    template<typename D> requires std::is_trivially_copyable_v<D> && (!std::is_same_v<D, uint8_t>)
    void add_mesh(const std::string &key, const Resource::VertexLayout& vertex_layout, std::span<D> vertices, std::span<const uint16_t> indices, Topology topology = Topology::TRIANGLE){
        add_mesh(key, vertex_layout, std::span((uint8_t* const)vertices.data(), vertices.size_bytes()), indices, topology);
    }

    void add_material(const std::string &key, const Resource::MaterialDesc &desc) {
        LOG_INFO("Loading Material: {}", key);
        std::vector<intrusive_ptr<Texture>> tx;
        for(const auto& s: desc.samplers){
            tx.emplace_back(textures.at(s.texture_key));
        }
        materials.emplace(key, graphics_api->load_material(desc, tx));
    }
    void add_texture(const std::string &key, const std::string &image_path, bool is_color = false) {
        LOG_INFO("Loading Texture: {}", key);
        textures.emplace(key, graphics_api->load_texture2D(image_path, is_color));
    }
    void add_cubemap(const std::string &key, const std::string &image_px, const std::string &image_nx,
                     const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                     const std::string &image_nz) {
        LOG_INFO("Loading CubeMap: {}", key);
        textures.emplace(key, graphics_api->load_cubemap(image_px, image_nx, image_py, image_ny, image_pz, image_nz));
    };
    void add_shader(const std::string &key, const std::string &vs_path, const std::string &ps_path) {
        LOG_INFO("Loading Shader: {}", key);
        graphics_api->load_uber_shader(key, UberShaderDesc{vs_path, ps_path});
    }
};

extern RenderReousce resources;

} // namespace Graphics
} // namespace Goonya