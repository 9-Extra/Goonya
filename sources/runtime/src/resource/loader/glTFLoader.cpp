#include "glTFLoader.h"

#include "core/RefCount.h"
#include "core/as_u8string.h"
#include "core/cgmath/cgmath.h"
#include "core/path_formatter.h"
#include "function/animation/Animation.h"
#include "function/components/CpntMeshRender.h"
#include "function/renderer/Material.h"
#include "function/renderer/Mesh.h"
#include "function/renderer/UberShader.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/ResMng.h"
#include "resource/Resource.h"
#include <resource/loader/SceneLoader.h>

#include "runtime/GoonyaException.h"
#include "json/value.h"
#include <algorithm>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <json/json.h>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Goonya {

// 将单个转义三元组 %HH 解码成一个字节
static char hex_to_char(char c1, char c2) {
    auto hex = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        return -1; // 非法十六进制字符
    };
    int h1 = hex(c1);
    int h2 = hex(c2);
    if (h1 < 0 || h2 < 0) return '\0'; // 非法则返回 0
    return static_cast<char>((h1 << 4) | h2);
}

static std::string uri_decode(const std::string &src) {
    std::string out;
    out.reserve(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '%' && i + 2 < src.size()) {
            char decoded = hex_to_char(src[i + 1], src[i + 2]);
            if (decoded) // 只有合法才跳转，否则原样保留 '%'
            {
                out += decoded;
                i += 2;
                continue;
            }
        }
        // 其余字符（包括未转义的 UTF-8 多字节）原样保留
        out += src[i];
    }
    return out;
}

struct GlTFLoadingContext {
    Ref<ResourcePack> pack;
    std::filesystem::path path;
    Json::Value json;

    std::vector<Ref<GLTexture>> texture_list;
    std::vector<Ref<Material>> material_list;

    // 节点索引到路径的映射（假设节点名称不重复）
    std::vector<std::string> node_index_to_path;

    // 原始缓冲区数据（用于读取 accessor）
    std::vector<std::unique_ptr<char[]>> buffer_data;
    std::vector<size_t> buffer_sizes;

    GlTFLoadingContext(Ref<ResourcePack> pack, std::filesystem::path path)
        : pack(std::move(pack)), path(std::move(path)) {

        Json::Reader reader;
        std::ifstream file(this->path);
        if (!file.is_open()) {
            throw RuntimeError(std::format("打开文件{}失败", this->path));
        }
        reader.parse(file, json, false);

        // 预加载所有缓冲区数据
        load_buffers();
    }

    void load_buffers() {
        std::filesystem::path root = path.parent_path();
        if (!json.isMember("buffers")) {
            return;
        }
        const Json::Value &buffers_json = json["buffers"];
        buffer_data.reserve(buffers_json.size());
        buffer_sizes.reserve(buffers_json.size());

        for (const Json::Value &buffer : buffers_json) {
            std::filesystem::path bin_path = root / as_u8string_view(uri_decode(buffer["uri"].asString()));
            std::fstream file(bin_path, std::ios_base::in | std::ios_base::binary);
            if (!file) {
                throw RuntimeError(
                    std::format("打开文件{}失败", std::filesystem::canonical(bin_path).generic_string()));
            }

            size_t byte_length = buffer["byteLength"].asUInt();
            auto data = std::make_unique<char[]>(byte_length);
            file.read(data.get(), byte_length);

            buffer_data.push_back(std::move(data));
            buffer_sizes.push_back(byte_length);
        }
    }

    // 获取 accessor 数据的指针和数量
    template <typename T>
    std::pair<const T *, size_t> get_accessor_data(uint32_t accessor_index) const {
        const Json::Value &accessor = json["accessors"][accessor_index];
        const Json::Value &buffer_view = json["bufferViews"][accessor["bufferView"].asUInt()];

        uint32_t buffer_index = buffer_view["buffer"].asUInt();
        size_t byte_offset = buffer_view.get("byteOffset", 0).asUInt();
        byte_offset += accessor.get("byteOffset", 0).asUInt();

        size_t count = accessor["count"].asUInt();
        const char *data_ptr = buffer_data[buffer_index].get() + byte_offset;

        return {reinterpret_cast<const T *>(data_ptr), count};
    }

    void load_gltf_mesh() {
        std::filesystem::path root = path.parent_path();

        struct Buffer {
            char *ptr = nullptr;
            size_t len;
            explicit Buffer(size_t len) : ptr(new char[len]), len(len) {}
            ~Buffer() { delete[] ptr; }
        };
        std::vector<Buffer> buffers;
        if (json.isMember("buffers")) {
            for (const Json::Value &buffer : json["buffers"]) {
                std::filesystem::path bin_path = root / as_u8string_view(uri_decode(buffer["uri"].asString()));
                std::fstream file(bin_path, std::ios_base::in | std::ios_base::binary);
                if (!file) {
                    throw RuntimeError(
                        std::format("打开文件{}失败", std::filesystem::canonical(bin_path).generic_string()));
                }

                Buffer &b = buffers.emplace_back(buffer["byteLength"].asUInt());
                file.read(b.ptr, b.len);
            }
        }
        auto get_buffer = [&](uint32_t accessor_id) -> const Json::Value & {
            const Json::Value &buffer_view = json["bufferViews"][json["accessors"][accessor_id]["bufferView"].asUInt()];
            return buffer_view;
        };
        // 加载网格

        for (const Json::Value &mesh : json["meshes"]) {
            const std::string &key = mesh["name"].asString();
            // 把mesh内部的primitives看作submesh，把所有primitive拼成一个大mesh
            struct PrimitiveInfo {
                Vector3f *pos;
                Vector3f *normal;
                Vector2f *uv;
                Vector4f *tangent;
                uint32_t vertex_count;

                uint16_t *indices_ptr;
                uint32_t indices_count;

                int32_t material_id;

                Vector3f pos_min;
                Vector3f pos_max;
            };
            std::vector<PrimitiveInfo> primitive_info; // 收集所有Primitive信息并存在这里
            primitive_info.reserve(mesh["primitives"].size());
            uint32_t total_vertex_count = 0; // 同时计算总顶点数和总索引数
            uint32_t total_indices_count = 0;

            for (const Json::Value &primitive : mesh["primitives"]) {
                // 解析各个顶点属性
                const Json::Value &indices_buffer = get_buffer(primitive["indices"].asUInt());
                const Json::Value &position_buffer = get_buffer(primitive["attributes"]["POSITION"].asUInt());
                const Json::Value &normal_buffer = get_buffer(primitive["attributes"]["NORMAL"].asUInt());
                const Json::Value &uv_buffer = get_buffer(primitive["attributes"]["TEXCOORD_0"].asUInt());
                const Json::Value &tangent_buffer = get_buffer(primitive["attributes"]["TANGENT"].asUInt());

                // 解析索引
                const Json::Value &indices_accessor = json["accessors"][primitive["indices"].asUInt()];
                uint32_t indices_count = indices_accessor["count"].asUInt();
                GN_ASSERT(indices_count % 3 == 0);
                GN_ASSERT(indices_accessor["componentType"].asUInt() == 5123); // 保证索引类型是uint16_t
                uint16_t *indices_ptr = reinterpret_cast<uint16_t *>(buffers[indices_buffer["buffer"].asUInt()].ptr +
                                                                     indices_buffer["byteOffset"].asUInt());

                // 解析材质，按glTF规定，如果材质未制定，则使用所有参数取默认值的glTF材质
                // 与glTF不同，Goonya的Mesh内没有材质信息，在MeshRenderProxy中才有
                int32_t material_id = primitive.get("material", -1).asInt();

                // 获取所有顶点属性的数量，显然它们应当相同
                uint32_t vertex_count = position_buffer["byteLength"].asUInt() / sizeof(Vector3f);
                uint32_t normal_count = normal_buffer["byteLength"].asUInt() / sizeof(Vector3f);
                uint32_t uv_count = uv_buffer["byteLength"].asUInt() / sizeof(Vector2f);
                uint32_t tangent_count = tangent_buffer["byteLength"].asUInt() / sizeof(Vector4f);
                if (vertex_count == 0 || vertex_count != normal_count || vertex_count != uv_count ||
                    vertex_count != tangent_count) {
                    throw RuntimeError(std::format("gltf文件\"{}\"网格数据格式不对", path.string()));
                }
                // 获取指向顶点数据的指针
                Vector3f *pos = reinterpret_cast<Vector3f *>(buffers[position_buffer["buffer"].asUInt()].ptr +
                                                             position_buffer["byteOffset"].asUInt());
                Vector3f *normal = reinterpret_cast<Vector3f *>(buffers[normal_buffer["buffer"].asUInt()].ptr +
                                                                normal_buffer["byteOffset"].asUInt());
                Vector2f *uv = reinterpret_cast<Vector2f *>(buffers[uv_buffer["buffer"].asUInt()].ptr +
                                                            uv_buffer["byteOffset"].asInt64());
                Vector4f *tangent = reinterpret_cast<Vector4f *>(buffers[tangent_buffer["buffer"].asUInt()].ptr +
                                                                 tangent_buffer["byteOffset"].asUInt());
                // 计算顶点位置的最大最小值，用于包围盒
                // todo: 尝试从glsl读取
                Vector3f position_min = pos[0];
                Vector3f position_max = pos[0];
                for (uint32_t i = 0; i < vertex_count; i++) {
                    position_min = {std::min(pos[i].x, position_min.x), std::min(pos[i].y, position_min.y),
                                    std::min(pos[i].z, position_min.z)};
                    position_max = {std::max(pos[i].x, position_max.x), std::max(pos[i].y, position_max.y),
                                    std::max(pos[i].z, position_max.z)};
                }
                // 收集所有数据到primitive_info中
                primitive_info.emplace_back(PrimitiveInfo{pos, normal, uv, tangent, vertex_count, indices_ptr,
                                                          indices_count, material_id, position_min, position_max});
                // 统计整个Mesh的顶点和索引总数，用于计算一次性分配Buffer的大小
                total_vertex_count += vertex_count;
                total_indices_count += indices_count;
            }

            // 使用MeshBuilder构建网格
            MeshDataArrays mesh_builder;
            mesh_builder.position.reserve(total_vertex_count);
            mesh_builder.normal.reserve(total_vertex_count);
            mesh_builder.tangent.reserve(total_vertex_count);
            mesh_builder.uv.reserve(total_vertex_count);
            mesh_builder.indices.reserve(total_indices_count);
            mesh_builder.submeshes.emplace().reserve(primitive_info.size());

            uint32_t vertex_offset = 0;
            uint32_t index_offset = 0;
            // 遍历之前收集到的PrimitiveInfo，将其中数据填入MeshBuilder
            for (const PrimitiveInfo &info : primitive_info) {

                for (uint32_t i = 0; i < info.vertex_count; i++) {
                    mesh_builder.position.push_back(info.pos[i]);
                    mesh_builder.normal.push_back(info.normal[i]);
                    mesh_builder.tangent.push_back(info.tangent[i]);
                    mesh_builder.uv.push_back(info.uv[i]);
                }

                for (uint32_t i = 0; i < info.indices_count; i += 3) {
                    // 将顶点环绕方向从gltf的逆时针反转为Goonya定义的顺时针
                    mesh_builder.indices.push_back(info.indices_ptr[i + 2]);
                    mesh_builder.indices.push_back(info.indices_ptr[i + 1]);
                    mesh_builder.indices.push_back(info.indices_ptr[i + 0]);
                }

                mesh_builder.submeshes->emplace_back(SubMesh{index_offset, info.indices_count, vertex_offset,
                                                             Topology::TRIANGLE,
                                                             BoundingBox{info.pos_min, info.pos_max}});

                vertex_offset += info.vertex_count;
                index_offset += info.indices_count;
            }

            GN_ASSERT(vertex_offset == total_vertex_count && index_offset == total_indices_count);
            // 用收集完成的数据构建GLMesh并添加资源
            Ref<Mesh> device_mesh = create_ref<Mesh>(mesh_builder);
            pack->contents.emplace(key, device_mesh);
        }
    }

    void load_gltf_material() {
        std::filesystem::path root = path.parent_path();

        // 加载纹理（在加载材质时加载需要的纹理）
        auto load_texture = [&](uint32_t index, bool is_color) -> Ref<GLTexture> {
            if (index < texture_list.size() && texture_list[index]) {
                return texture_list[index];
            }

            const Json::Value &texture_info = json["textures"][index];
            const Json::Value &image_info = json["images"][texture_info["source"].asUInt()];
            const std::string key = image_info["name"].asString();
            const Json::Value &sampler_info = json["samplers"][texture_info["sampler"].asUInt()];

            std::filesystem::path image_path = root / as_u8string_view(uri_decode(image_info["uri"].asString()));

            stb::Image image = stb::Image::load(image_path, is_color);
            if (!image) {
                throw RuntimeError(std::format("图像{}加载失败", image_path));
            }

            uint32_t width = image.get_width();
            uint32_t height = image.get_height();

            TextureStorageFormat storage_type = GLTexture::get_proper_storage_type(image);

            if (storage_type == TextureStorageFormat::UNKNOWN) {
                throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", image_path));
            }

            Ref<GLTexture> texture =
                create_ref<GLTexture>(TextureType::TEXTURE_2D, storage_type, std::make_tuple(width, height, 0));

            TextureFilterMode filter_mode = TextureFilterMode::BILINEAR;
            // 不严格支持glTF标准
            if (sampler_info.isMember("magFilter")) {
                uint32_t value = sampler_info["magFilter"].asUInt();
                if (value == 9728) {
                    filter_mode = TextureFilterMode::NEAREST;
                } else if (value == 9729) {
                    filter_mode = TextureFilterMode::BILINEAR;
                } else {
                    throw RuntimeError(std::format("无效的sampler.magFilter: {}", value));
                }
            }

            if (sampler_info.isMember("minFilter")) {
                uint32_t value = sampler_info["minFilter"].asUInt();
                if (value == 9728) {
                    filter_mode = TextureFilterMode::NEAREST;
                } else if (value == 9729) {
                    filter_mode = TextureFilterMode::BILINEAR;
                } else if (value == 9987 || value == 9984 || value == 9985 || value == 9986) {
                    filter_mode = TextureFilterMode::TRILINEAR; // 不严格
                } else {
                    throw RuntimeError(std::format("无效的sampler.minFilter: {}", value));
                }
            }

            texture->set_filter_mode(filter_mode);
            texture->import_image(image, 0);
            texture->generate_mipmaps();

            pack->contents.emplace(key, texture);
            if (texture_list.size() <= index) {
                texture_list.resize((index + 1) * 2);
            }
            texture_list[index] = texture;
            return texture;
        };

        // 加载材质
        for (const Json::Value &material : json["materials"]) {
            const std::string &key = material["name"].asString();
            Ref<Material> device_material =
                create_ref<Material>(resources.load_resource<UberShader>("shaders/pbr/pbr").get());

            Ref<GLTexture> basecolor_texture;
            if (material["pbrMetallicRoughness"].isMember("baseColorTexture")) {
                basecolor_texture =
                    load_texture(material["pbrMetallicRoughness"]["baseColorTexture"]["index"].asUInt(), true);
            } else {
                basecolor_texture = resources.load_resource<GLTexture>("buildin:missing_texture");
            }
            device_material->set_texture("basecolor_texture", basecolor_texture);

            if (material.isMember("normalTexture")) {
                device_material->set_local_variant_key("USE_NORMAL_MAP");
                Ref<GLTexture> normal_texture = load_texture(material["normalTexture"]["index"].asUInt(), false);
                device_material->set_texture("normal_texture", normal_texture);
            }

            if (material["pbrMetallicRoughness"].isMember("metallicRoughnessTexture")) {
                device_material->set_local_variant_key("USE_ORM_TEXTURE");
                Ref<GLTexture> orm_texture =
                    load_texture(material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"].asUInt(), false);
                device_material->set_texture("orm_texture", orm_texture);
            }

            float metallicFactor = material["pbrMetallicRoughness"].get("metallicFactor", 1.0).asFloat();
            float roughnessFactor = material["pbrMetallicRoughness"].get("roughnessFactor", 1.0).asFloat();
            device_material->set_param("metallic_factor", metallicFactor);
            device_material->set_param("roughness_factor", roughnessFactor);

            pack->contents.emplace(key, device_material);
            material_list.emplace_back(device_material);
        }
    }

    // 递归构建节点索引到路径的映射
    void build_node_path_map(uint32_t node_index, const std::string &parent_path) {
        if (node_index >= node_index_to_path.size()) {
            node_index_to_path.resize(node_index + 1);
        }

        const Json::Value &node_json = json["nodes"][node_index];
        std::string name = node_json.get("name", "").asString();
        std::string current_path = parent_path.empty() ? name : parent_path + "/" + name;
        node_index_to_path[node_index] = current_path;

        // 递归处理子节点
        if (node_json.isMember("children")) {
            for (const Json::Value &child_index : node_json["children"]) {
                build_node_path_map(child_index.asUInt(), current_path);
            }
        }
    }

    // 初始化节点路径映射（从场景根节点开始）
    void init_node_path_map() {
        if (!json.isMember("scenes")) {
            return;
        }
        for (const Json::Value &scene_json : json["scenes"]) {
            if (scene_json.isMember("nodes")) {
                for (const Json::Value &node_index : scene_json["nodes"]) {
                    build_node_path_map(node_index.asUInt(), "");
                }
            }
        }
    }

    std::shared_ptr<GObject> load_gltf_node(uint32_t index) {
        const Json::Value &node_json = json["nodes"][index];
        // 先加载名称和变换
        std::string name = node_json.get("name", "").asString();
        Transform transform;
        if (node_json.isMember("translation")) {
            transform.position = Vector3f{node_json["translation"][0].asFloat(), node_json["translation"][1].asFloat(),
                                          node_json["translation"][2].asFloat()};
        }
        if (node_json.isMember("rotation")) {
            transform.rotation = Quaternion{node_json["rotation"][0].asFloat(), node_json["rotation"][1].asFloat(),
                                            node_json["rotation"][2].asFloat(), node_json["rotation"][3].asFloat()};
        }
        if (node_json.isMember("scale")) {
            transform.scale = Vector3f{node_json["scale"][0].asFloat(), node_json["scale"][1].asFloat(),
                                       node_json["scale"][2].asFloat()};
        }
        // 构造GObject对象
        std::shared_ptr<GObject> node = std::make_shared<GObject>(transform, name);

        // 加载额外属性
        if (node_json.isMember("mesh")) {

            std::unique_ptr<CpntMeshRender> mesh_render = std::make_unique<CpntMeshRender>();

            // 加载Mesh
            uint32_t mesh_id = node_json["mesh"].asUInt();
            const Json::Value &mesh_json = json["meshes"][mesh_id];
            AssetKey mesh_key = mesh_json["name"].asString();
            Ref<Mesh> mesh = Ref<Mesh>::cast_from(pack->contents.at(mesh_key));
            mesh_render->set_mesh(mesh);

            /*
            在GTLF中mesh属性中包含了其绑定的每一个材质，在加载时写入了material_list，因此从这里获取材质
            */
            for (const auto &[id, primetive_json] : std::ranges::enumerate_view(mesh_json["primitives"])) {
                if (primetive_json.isMember("material")) {
                    uint32_t material_id = primetive_json["material"].asUInt();
                    mesh_render->set_material(id, material_list.at(material_id));
                } else {
                    // 应该使用默认的glTF材质
                    // todo
                }
            }

            node->add_component(std::move(mesh_render));
        }
        if (node_json.isMember("skin")) {
            // todo
        }

        // 加载子节点
        for (const Json::Value &node_index : node_json["children"]) {
            node->attach_child(load_gltf_node(node_index.asUInt()));
        }

        return node;
    }

    void load_gltf_scene() {
        // 首先初始化节点路径映射
        init_node_path_map();

        for (const auto &[id, scene_json] : std::ranges::enumerate_view(json["scenes"])) {
            Ref<Scene> scene = create_ref<Scene>();
            // 默认使用其自定义的名称，否则使用下标生成名称，重名会导致错误
            if (scene_json.isMember("name")) {
                scene->name = scene_json["name"].asString();
            } else {
                scene->name = std::format("scene_{}", id);
            }
            AssetKey res_name = scene->name;

            for (const Json::Value &node_index : scene_json["nodes"]) {
                scene->nodes.emplace_back(load_gltf_node(node_index.asUInt()));
            }
            pack->contents.emplace(std::move(res_name), std::move(scene));
        }
    }

    void load_gltf_animation() {
        if (!json.isMember("animations")) {
            return;
        }

        const Json::Value &animations_json = json["animations"];

        for (const Json::Value &anim_json : animations_json) {
            Ref<Animation> animation = create_ref<Animation>();

            std::string anim_name = anim_json.get("name", "animation").asString();
            float max_duration = 0.0f;

            // 解析 samplers
            const Json::Value &samplers_json = anim_json["samplers"];

            // 解析 channels
            const Json::Value &channels_json = anim_json["channels"];

            for (const Json::Value &channel_json : channels_json) {
                // 获取 sampler 索引
                uint32_t sampler_index = channel_json["sampler"].asUInt();
                const Json::Value &sampler_json = samplers_json[sampler_index];

                // 获取 target 信息
                const Json::Value &target_json = channel_json["target"];

                // 如果 node 未定义，忽略此通道
                if (!target_json.isMember("node")) {
                    continue;
                }

                uint32_t node_index = target_json["node"].asUInt();
                std::string path = target_json["path"].asString();

                // 获取节点路径（假设节点名称不重复）
                if (node_index >= node_index_to_path.size()) {
                    continue; // 节点索引越界
                }
                std::string node_path = node_index_to_path[node_index];
                if (node_path.empty()) {
                    continue; // 无法找到有效路径
                }

                // 获取 interpolation 类型（CUBICSPLINE 转为 LINEAR）
                std::string interpolation_str = sampler_json.get("interpolation", "LINEAR").asString();
                InterpolationType interp_type = InterpolationType::LINEAR;
                if (interpolation_str == "STEP") {
                    interp_type = InterpolationType::STEP;
                }
                // CUBICSPLINE 也使用 LINEAR

                // 获取 sampler 的 input/output accessor
                uint32_t input_accessor = sampler_json["input"].asUInt();
                uint32_t output_accessor = sampler_json["output"].asUInt();

                // 读取时间数据（input）
                auto [time_data, time_count] = get_accessor_data<float>(input_accessor);
                if (time_count == 0) {
                    continue;
                }

                // 更新最大持续时间
                max_duration = std::max(max_duration, time_data[time_count - 1]);

                // 根据 path 类型创建对应的 Channel
                if (path == "translation") {
                    auto [pos_data, pos_count] = get_accessor_data<Vector3f>(output_accessor);
                    if (pos_count != time_count) {
                        continue; // 数据不匹配
                    }

                    auto pos_channel = std::make_unique<Animation::PositionChannel>();
                    pos_channel->target = node_path;
                    pos_channel->interpolation_type = interp_type;

                    pos_channel->position_series.key_points.reserve(time_count);
                    for (size_t i = 0; i < time_count; ++i) {
                        pos_channel->position_series.key_points.push_back(
                            KeyPoint<Vector3f>{time_data[i], pos_data[i]});
                    }

                    animation->channels.push_back(std::move(pos_channel));

                } else if (path == "rotation") {
                    // rotation 使用 VEC4 (xyzw)
                    auto [rot_data, rot_count] = get_accessor_data<Vector4f>(output_accessor);
                    if (rot_count != time_count) {
                        continue;
                    }

                    auto rot_channel = std::make_unique<Animation::RotationChannel>();
                    rot_channel->target = node_path;
                    rot_channel->interpolation_type = interp_type;

                    rot_channel->rotation_series.key_points.reserve(time_count);
                    for (size_t i = 0; i < time_count; ++i) {
                        Quaternion quat(rot_data[i].x, rot_data[i].y, rot_data[i].z, rot_data[i].w);
                        rot_channel->rotation_series.key_points.push_back(KeyPoint<Quaternion>{time_data[i], quat});
                    }

                    animation->channels.push_back(std::move(rot_channel));

                } else if (path == "scale") {
                    auto [scale_data, scale_count] = get_accessor_data<Vector3f>(output_accessor);
                    if (scale_count != time_count) {
                        continue;
                    }

                    auto scale_channel = std::make_unique<Animation::ScaleChannel>();
                    scale_channel->target = node_path;
                    scale_channel->interpolation_type = interp_type;

                    scale_channel->scale_series.key_points.reserve(time_count);
                    for (size_t i = 0; i < time_count; ++i) {
                        scale_channel->scale_series.key_points.push_back(
                            KeyPoint<Vector3f>{time_data[i], scale_data[i]});
                    }

                    animation->channels.push_back(std::move(scale_channel));
                }
                // "weights" 路径被忽略（Animation 类不支持）
            }

            // 设置动画持续时间（秒转换为 GameClock::Duration）
            animation->duration =
                std::chrono::duration_cast<GameClock::Duration>(std::chrono::duration<float>(max_duration));

            // 只有当有有效通道时才添加动画
            if (!animation->channels.empty()) {
                pack->contents.emplace(anim_name, animation);
            }
        }
    }
};

Ref<Resource> GlTFLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                               const Json::Value &content) {

    if (!content.isMember("file")) {
        throw RuntimeError("缺少content.file字段");
    }

    Ref<ResourcePack> pack = create_ref<ResourcePack>();
    std::filesystem::path gltf_file_path = base_dir / as_u8string_view(content["file"].asString());
    GlTFLoadingContext context{pack, gltf_file_path};

    context.load_gltf_material();
    context.load_gltf_mesh();
    context.load_gltf_scene();
    context.load_gltf_animation();

    return pack;
}

} // namespace Goonya
