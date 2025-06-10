#include "glTFLoader.h"

#include "core/Bytes.h"
#include "core/cgmath.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/Texture.h"
#include "resource/GraphicsResourceBuilder.h"
#include "resource/Resource.h"

#include "runtime/GoonyaException.h"
#include <cassert>
#include <cstddef>
#include <fstream>
#include <json/json.h>
#include <vector>

namespace Goonya::Resource {

void load_gltf(const AssetKey &base_key, const std::filesystem::path &path) {
    std::filesystem::path root = path.parent_path();

    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        reader.parse(file, json, false);
    }

    struct Buffer {
        char *ptr = nullptr;
        size_t len;
        explicit Buffer(size_t len) : ptr(new char[len]), len(len) {}
        ~Buffer() { delete[] ptr; }
    };
    std::vector<Buffer> buffers;
    if (json.isMember("buffers")) {
        for (const Json::Value &buffer : json["buffers"]) {
            std::filesystem::path bin_path = root / buffer["uri"].asString();
            Buffer &b = buffers.emplace_back(buffer["byteLength"].asUInt());
            std::fstream file(bin_path, std::ios_base::in | std::ios_base::binary);
            file.read(b.ptr, b.len);
        }
    }
    auto get_buffer = [&](uint32_t accessor_id) -> const Json::Value & {
        const Json::Value &buffer_view = json["bufferViews"][json["accessors"][accessor_id]["bufferView"].asUInt()];
        return buffer_view;
    };
    // 加载网格
    if (json.isMember("meshes")) {
        for (const Json::Value &mesh : json["meshes"]) {
            const std::string &key = base_key + '.' + mesh["name"].asString();
            const Json::Value &primitive = mesh["primitives"][0];
            const Json::Value &indices_buffer = get_buffer(primitive["indices"].asUInt());
            const Json::Value &position_buffer = get_buffer(primitive["attributes"]["POSITION"].asUInt());
            const Json::Value &normal_buffer = get_buffer(primitive["attributes"]["NORMAL"].asUInt());
            const Json::Value &uv_buffer = get_buffer(primitive["attributes"]["TEXCOORD_0"].asUInt());
            const Json::Value &tangent_buffer = get_buffer(primitive["attributes"]["TANGENT"].asUInt());

            uint32_t indices_count = json["accessors"][primitive["indices"].asUInt()]["count"].asUInt();
            assert(indices_count % 3 == 0);
            uint16_t *indices_ptr = reinterpret_cast<uint16_t *>(buffers[indices_buffer["buffer"].asUInt()].ptr +
                                                                 indices_buffer["byteOffset"].asUInt());

            uint32_t vertex_count = position_buffer["byteLength"].asUInt() / sizeof(Vector3f);
            uint32_t normal_count = normal_buffer["byteLength"].asUInt() / sizeof(Vector3f);
            uint32_t uv_count = uv_buffer["byteLength"].asUInt() / sizeof(Vector2f);
            uint32_t tangent_count = tangent_buffer["byteLength"].asUInt() / sizeof(Vector4f);
            if (vertex_count != normal_count || vertex_count != uv_count || vertex_count != tangent_count) {
                throw RuntimeError(std::format("gltf文件\"{}\"网格数据格式不对", path.string()));
            }
            Vector3f *pos = reinterpret_cast<Vector3f *>(buffers[position_buffer["buffer"].asUInt()].ptr +
                                                         position_buffer["byteOffset"].asUInt());
            Vector3f *normal = reinterpret_cast<Vector3f *>(buffers[normal_buffer["buffer"].asUInt()].ptr +
                                                            normal_buffer["byteOffset"].asUInt());
            Vector2f *uv = reinterpret_cast<Vector2f *>(buffers[uv_buffer["buffer"].asUInt()].ptr +
                                                        uv_buffer["byteOffset"].asInt64());
            Vector4f *tangent = reinterpret_cast<Vector4f *>(buffers[tangent_buffer["buffer"].asUInt()].ptr +
                                                             tangent_buffer["byteOffset"].asUInt());

            struct Vertex {
                Vector3f position;
                Vector3f normal;
                Vector3f tangent;
                Vector2f uv;
            };
            Bytes raw_vertices(vertex_count * sizeof(Vertex));
            std::span<Vertex> vertices = raw_vertices.as_span<Vertex>();
            for (uint32_t i = 0; i < vertex_count; i++) {
                // tangent的第四个分量是用来根据平台决定手性的，在opengl中始终应该取1，所以忽略
                Vector3f tang = Vector3f(tangent[i].x, tangent[i].y, tangent[i].z);
                vertices[i] = {pos[i], normal[i], tang, uv[i]};
            }

            // 将顶点环绕方向从gltf的逆时针反转为Goonya定义的顺时针
            std::vector<uint16_t> indices(indices_count);
            for (uint32_t i = 0; i < indices_count; i += 3) {
                indices[i + 0] = indices_ptr[i + 2];
                indices[i + 1] = indices_ptr[i + 1];
                indices[i + 2] = indices_ptr[i + 0];
            }

            const static Graphics::VertexLayout vertex_layout{
                {{Graphics::VertexAttribute::POSITION, Meta::FieldType::vec3f, offsetof(Vertex, position)},
                 {Graphics::VertexAttribute::NORMAL, Meta::FieldType::vec3f, offsetof(Vertex, normal)},
                 {Graphics::VertexAttribute::TANGENT, Meta::FieldType::vec3f, offsetof(Vertex, tangent)},
                 {Graphics::VertexAttribute::UV, Meta::FieldType::vec2f, offsetof(Vertex, uv)}},
                sizeof(Vertex)};

            resources.meshes.add(
                key, Graphics::MeshDesc{vertex_layout, std::move(raw_vertices), indices, Graphics::Topology::TRIANGLE});
        }
    }
    // 加载纹理（在加载材质时加载需要的纹理）
    auto load_texture = [&](uint32_t index, bool is_color) -> std::string {
        const Json::Value &texture_info = json["textures"][index];
        const Json::Value &image_info = json["images"][texture_info["source"].asUInt()];
        const Json::Value &sampler_info = json["samplers"][texture_info["sampler"].asUInt()];
        const std::string key = base_key + '.' + image_info["name"].asString();
        Texture2DDesc desc = {.path = root / image_info["uri"].asString(), .is_color = is_color};
        // 不严格支持glTF标准
        if (sampler_info.isMember("magFilter")) {
            uint32_t value = sampler_info["magFilter"].asUInt();
            if (value == 9728) {
                desc.filter_mode = Graphics::TextureFilterMode::NEAREST;
            } else if (value == 9729) {
                desc.filter_mode = Graphics::TextureFilterMode::BILINEAR;
            } else {
                throw RuntimeError(std::format("无效的sampler.magFilter: {}", value));
            }
        }

        if (sampler_info.isMember("minFilter")) {
            uint32_t value = sampler_info["minFilter"].asUInt();
            if (value == 9728) {
                desc.filter_mode = Graphics::TextureFilterMode::NEAREST;
            } else if (value == 9729) {
                desc.filter_mode = Graphics::TextureFilterMode::BILINEAR;
            } else if (value == 9987 || value == 9984 || value == 9985 || value == 9986) {
                desc.filter_mode = Graphics::TextureFilterMode::TRILINEAR; // 不严格
            } else {
                throw RuntimeError(std::format("无效的sampler.minFilter: {}", value));
            }
        }

        resources.texture2ds.add(key, std::move(desc));
        return key;
    };

    // 加载材质
    if (json.isMember("materials")) {
        for (const Json::Value &material : json["materials"]) {
            const std::string &key = base_key + '.' + material["name"].asString();

            std::string normal_texture = "default_normal";
            if (material.isMember("normalTexture")) {
                normal_texture = load_texture(material["normalTexture"]["index"].asUInt(), false);
            }
            const std::string basecolor_texture =
                load_texture(material["pbrMetallicRoughness"]["baseColorTexture"]["index"].asUInt(), true);
            std::string metallic_roughness_texture = "black";
            if (material["pbrMetallicRoughness"].isMember("metallicRoughnessTexture")) {
                metallic_roughness_texture = load_texture(
                    material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"].asUInt(), false);
            }

            float metallicFactor = 1.0f;
            if (material["pbrMetallicRoughness"].isMember("metallicFactor")) {
                metallicFactor = material["pbrMetallicRoughness"]["metallicFactor"].asFloat();
            }
            float roughnessFactor = 1.0f;
            if (material["pbrMetallicRoughness"].isMember("roughnessFactor")) {
                roughnessFactor = material["pbrMetallicRoughness"]["roughnessFactor"].asFloat();
            }

            Resource::MaterialBuilder mat_builder("pbr");
            Graphics::MaterialDesc desc =
                mat_builder.add_parameter("metallic_factor", metallicFactor)
                    .add_parameter("roughness_factor", roughnessFactor)
                    .add_sampler("basecolor_texture", Graphics::TextureType::TEXTURE_2D, basecolor_texture)
                    .add_sampler("normal_texture", Graphics::TextureType::TEXTURE_2D, normal_texture)
                    .add_sampler("metallic_roughness_texture", Graphics::TextureType::TEXTURE_2D,
                                 metallic_roughness_texture)
                    .build();

            resources.materials.add(key, std::move(desc));
        }
    }
}

} // namespace Goonya::Resource
