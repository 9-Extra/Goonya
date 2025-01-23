#include "ResourceJsonLoader.h"

#include <fstream>
#include <json/json.h>

#include "GraphicsResourceBuilder.h"
#include "core/cgmath.h"
#include "function/renderer/RenderResource.h"
#include "platform/graphics/GraphicsResource.h"


namespace Goonya {
namespace Resource {

void load_json(const std::string &path) {
    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        if (!file) {
            throw RuntimeError(std::format("资源文件{}未找到", path));
        }
        reader.parse(file, json, false);
    }

    std::string base_dir;
    if (size_t it = path.find_last_of("/\\"); it != std::string::npos) {
        base_dir = path.substr(0, it + 1); // 包含'/'
    } else {
        base_dir = ""; // 可能在同一目录下
    }

    if (json.isMember("shader")) {
        for (const auto &key : json["shader"].getMemberNames()) {
            const Json::Value &shader_desc = json["shader"][key];
            Graphics::resources.add_shader(key, base_dir + shader_desc["vs_path"].asString(),
                                           base_dir + shader_desc["ps_path"].asString());
        }
    }

    if (json.isMember("texture")) {
        for (const auto &key : json["texture"].getMemberNames()) {
            const Json::Value &texture_desc = json["texture"][key];
            bool is_color = texture_desc.isMember("is_color") ? texture_desc["is_color"].asBool() : true;
            Graphics::resources.add_texture(key, base_dir + texture_desc["image"].asString(), is_color);
        }
    }

    if (json.isMember("cubemap")) {
        for (const auto &key : json["cubemap"].getMemberNames()) {
            const Json::Value &cubemap_desc = json["cubemap"][key];
            Graphics::resources.add_cubemap(
                key, base_dir + cubemap_desc["px"].asString(), base_dir + cubemap_desc["nx"].asString(),
                base_dir + cubemap_desc["py"].asString(), base_dir + cubemap_desc["ny"].asString(),
                base_dir + cubemap_desc["pz"].asString(), base_dir + cubemap_desc["nz"].asString());
        }
    }

    if (json.isMember("gltf")) {
        for (const auto &key : json["gltf"].getMemberNames()) {
            load_gltf(key, base_dir + json["gltf"][key]["path"].asString());
        }
    }

    // materials
    for (const auto &key : json["materials"].getMemberNames()) {
        const Json::Value &material_desc = json["materials"][key];

        const Json::Value &shader_desc_json = material_desc["pso"]["shader"];
        std::unordered_map<std::string, std::string> definations;
        for (const auto &key : shader_desc_json["definations"].getMemberNames()) {
            definations.emplace(key, shader_desc_json["definations"][key].asString());
        }

        Resource::PSOBuilder pso_builder;
        pso_builder.set_uber_shader(shader_desc_json["uber"].asString()).update_shader_defines(definations);

        const Json::Value &pso_config = material_desc["pso"]["config"];
        if (pso_config.isMember("depth_func")) {
            pso_builder.set_depth_func(pso_config["depth_func"].asString());
        }

        Resource::MaterialBuilder mat_builder;
        mat_builder.set_pso(pso_builder.build());

        for (const Json::Value &uniform_json : material_desc["constants"]) {
            mat_builder.add_uniform(uniform_json["slot"].asUInt(), uniform_json["size"].asUInt(), nullptr);
        }

        for (const Json::Value &sampler_json : material_desc["samplers"]) {
            mat_builder.add_sampler(
                sampler_json["slot"].asUInt(), sampler_json["name"].asString(),
                sampler_json.isMember("type") ? sampler_json["type"].asString() : "rgb",
                sampler_json.isMember("warp_mode") ? sampler_json["warp_mode"].asString() : "repeat",
                sampler_json.isMember("filter_mode") ? sampler_json["filter_mode"].asString() : "bilinear");
        }

        Graphics::resources.add_material(key, mat_builder.build());
    }
}

void load_gltf(const std::string &base_key, const std::string &path) {
    std::string root = path;
    for (; !(root.empty() || root.back() == '/' || root.back() == '\\'); root.pop_back())
        ;

    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        reader.parse(file, json, false);
    }

    struct Buffer {
        char *ptr = nullptr;
        size_t len;
        Buffer(size_t len) : ptr(new char[len]), len(len) {}
        ~Buffer() {
            if (ptr != nullptr) {
                delete[] ptr;
            }
        }
    };
    std::vector<Buffer> buffers;
    if (json.isMember("buffers")) {
        for (const Json::Value &buffer : json["buffers"]) {
            std::string bin_path = root + buffer["uri"].asString();
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
            const Json::Value &indices_buffer = get_buffer(primitive["indices"].asInt64());
            const Json::Value &position_buffer = get_buffer(primitive["attributes"]["POSITION"].asInt64());
            const Json::Value &normal_buffer = get_buffer(primitive["attributes"]["NORMAL"].asInt64());
            const Json::Value &uv_buffer = get_buffer(primitive["attributes"]["TEXCOORD_0"].asInt64());
            const Json::Value &tangent_buffer = get_buffer(primitive["attributes"]["TANGENT"].asInt64());

            uint32_t indices_count = json["accessors"][primitive["indices"].asUInt()]["count"].asInt64();
            uint16_t *indices_ptr = (uint16_t *)((char *)buffers[indices_buffer["buffer"].asInt64()].ptr +
                                                 indices_buffer["byteOffset"].asInt64());

            uint32_t vertex_count = position_buffer["byteLength"].asInt64() / sizeof(Vector3f);
            uint32_t normal_count = normal_buffer["byteLength"].asInt64() / sizeof(Vector3f);
            uint32_t uv_count = uv_buffer["byteLength"].asInt64() / sizeof(Vector2f);
            uint32_t tangent_count = tangent_buffer["byteLength"].asInt64() / sizeof(Vector4f);
            assert(vertex_count == normal_count && vertex_count == uv_count && vertex_count == tangent_count);
            Vector3f *pos = (Vector3f *)((char *)buffers[position_buffer["buffer"].asInt64()].ptr +
                                         position_buffer["byteOffset"].asInt64());
            Vector3f *normal = (Vector3f *)((char *)buffers[normal_buffer["buffer"].asInt64()].ptr +
                                            normal_buffer["byteOffset"].asInt64());
            Vector2f *uv =
                (Vector2f *)((char *)buffers[uv_buffer["buffer"].asInt64()].ptr + uv_buffer["byteOffset"].asInt64());
            Vector4f *tangent = (Vector4f *)((char *)buffers[tangent_buffer["buffer"].asInt64()].ptr +
                                             tangent_buffer["byteOffset"].asInt64());

            std::vector<Graphics::Vertex> vertices(vertex_count);
            for (uint32_t i = 0; i < vertex_count; i++) {
                // tangent的第四个分量是用来根据平台决定手性的，在opengl中始终应该取1，所以忽略
                Vector3f tang = Vector3f(tangent[i].x, tangent[i].y, tangent[i].z);
                vertices[i] = {pos[i], normal[i], tang, uv[i]};
            }

            const static Graphics::VertexLayout vertex_layout{
                {{0, "position", Meta::FieldType::vec3f, offsetof(Graphics::Vertex, position)},
                 {1, "normal", Meta::FieldType::vec3f, offsetof(Graphics::Vertex, normal)},
                 {2, "tangent", Meta::FieldType::vec3f, offsetof(Graphics::Vertex, tangent)},
                 {3, "uv", Meta::FieldType::vec2f, offsetof(Graphics::Vertex, uv)}},
                sizeof(Graphics::Vertex)};

            Graphics::resources.add_mesh(key, vertex_layout, std::span(vertices),
                                         std::span(indices_ptr, indices_count));
        }
    }
    // 加载纹理（在加载材质时加载需要的纹理）
    auto load_texture = [&](uint32_t index, bool is_color) -> std::string {
        const Json::Value &texture = json["images"][index];
        const std::string key = base_key + '.' + texture["name"].asString();
        Graphics::resources.add_texture(key, root + texture["uri"].asString(), is_color);
        return key;
    };

    // 加载材质
    if (json.isMember("materials")) {
        for (const Json::Value &material : json["materials"]) {
            const std::string &key = base_key + '.' + material["name"].asString();

            std::string normal_texture = "default_normal";
            if (material.isMember("normalTexture")) {
                normal_texture = load_texture(material["normalTexture"]["index"].asInt64(), false);
            }
            const std::string basecolor_texture =
                load_texture(material["pbrMetallicRoughness"]["baseColorTexture"]["index"].asInt64(), true);
            std::string metallic_roughness_texture = "white";
            if (material["pbrMetallicRoughness"].isMember("metallicRoughnessTexture")) {
                metallic_roughness_texture = load_texture(
                    material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"].asInt64(), false);
            }

            float metallicFactor = 1.0f;
            if (material["pbrMetallicRoughness"].isMember("metallicFactor")) {
                metallicFactor = material["pbrMetallicRoughness"]["metallicFactor"].asFloat();
            }
            float roughnessFactor = 1.0f;
            if (material["pbrMetallicRoughness"].isMember("roughnessFactor")) {
                roughnessFactor = material["pbrMetallicRoughness"]["roughnessFactor"].asFloat();
            }
            float uniform_data[2] = {metallicFactor, roughnessFactor};

            Resource::PSODesc pso = Resource::PSOBuilder().set_uber_shader("pbr").build();
            Resource::MaterialBuilder mat_builder;
            Resource::MaterialDesc desc = Resource::MaterialBuilder()
                                              .set_pso(pso)
                                              .add_uniform(2, sizeof(float) * 2, &uniform_data)
                                              .add_sampler(0, basecolor_texture)
                                              .add_sampler(1, normal_texture)
                                              .add_sampler(2, metallic_roughness_texture)
                                              .build();

            Graphics::resources.add_material(key, desc);
        }
    }
}

} // namespace Resource
} // namespace Goonya