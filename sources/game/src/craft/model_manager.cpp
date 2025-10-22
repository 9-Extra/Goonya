#include "model_manager.h"

#include "block/block.h"
#include "block/block_model.h"
#include "block/blockstate.h"
#include "block/blockstates.h"
#include "core/cgmath.h"
#include "core/log/Log.h"
#include "craft/core/core.h"
#include "craft/core/registry.h"
#include "craft/core/resource.h"
#include "json/config.h"
#include "json/value.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <regex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Craft {

std::optional<ModelManager> ModelManager::instance;

void ModelManager::initalize() {
    assert(BlockStateMap::get_blockstates().size() != 0); // 在BlockState初始化后
    assert(!instance.has_value());
    instance.emplace();
}

const BlockModel missing_block_model_unbaked{{{{0, 0, 0},
                                               {1, 1, 1},
                                               Goonya::Quaternion::identity(),
                                               {{{Direction::DOWN, {0, 0}, {1, 1}, ""},
                                                 {Direction::UP, {0, 0}, {1, 1}, ""},
                                                 {Direction::NORTH, {0, 0}, {1, 1}, ""},
                                                 {Direction::SOUTH, {0, 0}, {1, 1}, ""},
                                                 {Direction::EAST, {0, 0}, {1, 1}, ""},
                                                 {Direction::WEST, {0, 0}, {1, 1}, ""}}}

}}};

class BlockModelJsonLoader final {
private:
    std::filesystem::path resource_path;
    // 缓存所有访问过的Json文件防止反复读取解析，使用nullptr表示此文件读取已经失败过了
    std::unordered_map<std::string, std::unique_ptr<Json::Value>> json_model_cache;

public:
    explicit BlockModelJsonLoader(std::filesystem::path resource_path) : resource_path(std::move(resource_path)) {
        json_model_cache.emplace("", nullptr);
    }

    std::optional<BlockModel> load_block_model(const ResourceLocation &location) {
        LOG_TRACE("加载方块模型{}", location);
        const Json::Value *json = fetch_model_json(std::format("{}", location));
        if (!json) {
            return std::nullopt;
        } else {
            BlockModel model;
            parse_elements_recursive(*json, *json, model);
            return model;
        }
    }

private:
    // 返回空指针表示没有，对空字符串输入返回空指针，缓存所有访问过的Json文件防止反复读取解析
    const Json::Value *fetch_model_json(const std::string &location) {
        if (auto iter = json_model_cache.find(location); iter != json_model_cache.end()) {
            return iter->second.get();
        } else {
            ResourceLocation parsed_location = ResourceLocation::parse(location);
            std::filesystem::path path =
                resource_path / std::format("{}/models/{}.json", parsed_location.name_space, parsed_location.key);
            std::ifstream file(path, std::ios::binary | std::ios::in);
            if (!file) {
                LOG_ERROR("读取BlockModel\"{}\"失败, 文件{}不存在", location, path.generic_string());
                json_model_cache.emplace(location, nullptr);
                return nullptr;
            }

            std::unique_ptr<Json::Value> meta = std::make_unique<Json::Value>();
            if (!Json::parseFromStream(Json::CharReaderBuilder(), file, meta.get(), nullptr)) {
                LOG_ERROR("读取BlockModel\"{}\"失败, 解析{}出错", location, path.generic_string());
                json_model_cache.emplace(location, nullptr);
                return nullptr;
            }

            Json::Value *ret = meta.get();
            json_model_cache.emplace(location, std::move(meta));
            return ret;
        }
    }

    std::string get_texture(const Json::Value &value, std::string name) {
        if (name.starts_with("#")) {
            name = name.substr(1);
        } else {
            // todo: 需要再确认一下这种情况应该如何处理
            assert(false);
        }

        const Json::Value *current = &value;
        do {
            const Json::Value &textures = (*current)["textures"];
            if (textures.isMember(name)) {
                // found
                std::string result = textures[name].asString();
                if (result.starts_with('#')) {
                    name = result.substr(1);
                    current = &value;
                    continue;
                } else {
                    return result;
                }
            } else {
                std::string parent_name = (*current)["parent"].asString();
                if (!parent_name.empty()) {
                    current = fetch_model_json(std::format("{}", ResourceLocation::parse(parent_name)));
                } else {
                    current = nullptr;
                }
            }
        } while (current != nullptr);

        LOG_ERROR("解析BlockModel失败, 无法获取texture:\"{}\"", name);
        return ""; // 指向缺省纹理
    }

    static Goonya::Vector3f parse_vector3f(const Json::Value &vec) noexcept {
        return {vec[0].asFloat(), vec[1].asFloat(), vec[2].asFloat()};
    }

    static Goonya::Vector4f parse_vector4f(const Json::Value &vec) noexcept {
        return {vec[0].asFloat(), vec[1].asFloat(), vec[2].asFloat(), vec[3].asFloat()};
    }

    static Direction parse_direction(const std::string &direction_name) noexcept {
        if (direction_name == "north") {
            return Direction::NORTH;
        } else if (direction_name == "east") {
            return Direction::EAST;
        } else if (direction_name == "south") {
            return Direction::SOUTH;
        } else if (direction_name == "west") {
            return Direction::WEST;
        } else if (direction_name == "up") {
            return Direction::UP;
        } else if (direction_name == "down") {
            return Direction::DOWN;
        }
        LOG_ERROR("方块名称{}无效", direction_name);
        return Direction::DOWN;
    }

    void parse_elements_recursive(const Json::Value &child, const Json::Value &value, BlockModel &model) {
        // 把父节点的elements放在前面
        const Json::Value *parent = fetch_model_json(value["parent"].asString());
        if (parent) {
            parse_elements_recursive(child, *parent, model);
        }

        for (const Json::Value &e : value["elements"]) {
            BlockElement &element = model.elements.emplace_back();
            element.from = parse_vector3f(e["from"]) / 16;
            element.to = parse_vector3f(e["to"]) / 16;
            for (const std::string &direction_name : e["faces"].getMemberNames()) {
                BlockElement::Face &face = element.faces.emplace_back();
                face.direction = parse_direction(direction_name);

                const Json::Value &face_info = e["faces"][direction_name];
                // uv字段
                if (face_info.isMember("uv")) {
                    const Goonya::Vector4f uv = parse_vector4f(face_info["uv"]) / 16;
                    face.uv_up_left = {uv[0], uv[1]};
                    face.uv_down_right = {uv[2], uv[3]};
                } else {
                    // todo:应该生成
                    face.uv_up_left = {0, 0};
                    face.uv_down_right = {1, 1};
                }

                // texture字段必须存在
                face.texture_key = get_texture(child, face_info["texture"].asString());

                // tintindex字段，没有此字段则表示不需要染色
                face.tintindex = face_info.get("tintindex", -1).asInt();
                // 忽略cullface信息
            }
        }
    }
};

static std::vector<std::tuple<BlockStateProperty *, BlockStatePropertyValue>>
parse_state_variant_key(const std::string &variant_key) {
    // variant_key形如 facing=east,half=bottom,shape=inner_left
    static const std::regex pattern(R"(([^=,]+)=([^,]*))");

    std::vector<std::tuple<BlockStateProperty *, BlockStatePropertyValue>> result;

    auto begin = std::sregex_iterator(variant_key.begin(), variant_key.end(), pattern);
    auto end = std::sregex_iterator();

    for (std::sregex_iterator i = begin; i != end; ++i) {

        const std::smatch &match = *i;
        BlockStateProperty *property = PROPERTY_REGISTRY.find_entry(match[1].str());
        if (property == nullptr) {
            LOG_ERROR("方块属性{}未注册", match[1].str());
            continue;
        }
        std::optional<BlockStatePropertyValue> value = property->from_string(match[2].str());
        if (!value.has_value()) {
            LOG_ERROR("方块属性{}中不包含属性{}", property->get_name(), match[1].str());
            continue;
        }
        result.emplace_back(property, value.value());
    }

    return result;
}

static bool
is_match_block_state(const std::vector<std::tuple<BlockStateProperty *, BlockStatePropertyValue>> &property_pairs,
                     BlockState *state) {
    return std::ranges::all_of(property_pairs, [state](const auto &pair) {
        const auto &[p, v] = pair;
        // 假定属性一定存在
        return state->get_property_raw_value(p) == v;
    });
}

void ModelManager::load_all_models() {
    assert(blockstate_model_map.empty());

    std::filesystem::path resource_path = "../assets";
    TextureArrayAllocator texture_allocator(resource_path, 16, 16);
    BlockModelJsonLoader model_loader(resource_path);

    missing_block_model = bake_model(missing_block_model_unbaked, texture_allocator);

    for (const auto &[block, _] : REGISTRY_BLOCK) {
        ResourceLocation location = ResourceLocation::parse(REGISTRY_BLOCK.find_key(block));
        std::filesystem::path path =
            resource_path / std::format("{}/blockstates/{}.json", location.name_space, location.key);
        std::ifstream file(path, std::ios::binary | std::ios::in);
        if (!file) {
            LOG_ERROR("读取ModelState\"{}\"失败, 文件{}不存在", location, path.generic_string());
            continue;
        }

        Json::Value blockstate_json;
        if (!Json::parseFromStream(Json::CharReaderBuilder(), file, &blockstate_json, nullptr)) {
            LOG_ERROR("读取ModelState\"{}\"失败, 解析{}出错", location, path.generic_string());
            continue;
        }

        if (blockstate_json.isMember("variants")) {
            const std::vector<std::unique_ptr<BlockState>> &states = block->get_possible_states();

            for (const Json::String &variant_key : blockstate_json["variants"].getMemberNames()) {
                const std::vector<std::tuple<BlockStateProperty *, BlockStatePropertyValue>> property_pairs =
                    parse_state_variant_key(variant_key);
                for (const std::unique_ptr<BlockState> &_state : states) {
                    BlockState *state = _state.get();
                    if (is_match_block_state(property_pairs, state)) {
                        if (blockstate_model_map.contains(state)) {
                            LOG_ERROR("Exception loading blockstate definition: '{}' for variant: '{}'", location,
                                      variant_key);
                            continue;
                        }
                        const Json::Value &variant_content_list = blockstate_json["variants"][variant_key];
                        // 对于多选一的模型，目前固定选择第一个
                        const Json::Value &variant_content =
                            variant_content_list.isArray() ? variant_content_list[0] : variant_content_list;
                        ResourceLocation model_location = ResourceLocation::parse(variant_content["model"].asString());
                        std::optional<BlockModel> model = model_loader.load_block_model(model_location);
                        if (!model) {
                            LOG_ERROR("加载方块模型{}失败", model_location);
                            model = missing_block_model_unbaked;
                        }

                        BlockModel &m = model.value();

                        // bool uvlock = variant_content.get("uvlock", false).asBool();
                        // int32_t rotation_x = variant_content.get("x", 0).asInt();
                        // int32_t rotation_y = variant_content.get("y", 0).asInt();

                        blockstate_model_map.emplace(state, bake_model(m, texture_allocator));
                    }
                }
                // todo
            }
        } else if (blockstate_json.isMember("multipart")) {
            // todo
        } else {
            // 读取出错了，所有状态为缺省模型
            LOG_ERROR("Neither 'variants' nor 'multipart' found");
        }
    }

    block_texture_array = texture_allocator.generate_texture_array();
}

BakedBlockModel ModelManager::bake_model(const BlockModel &model_src, TextureArrayAllocator &texture_allocator) {
    using namespace Goonya;

    BakedBlockModel baked_model;

    for (const auto &element : model_src.elements) {
        const Vector3f &min = element.from;
        const Vector3f &max = element.to;

        for (const BlockElement::Face &face : element.faces) {

            BakedQuad quad;
            quad.normal = face.direction;
            quad.color_texture_index = texture_allocator.alloc_texture(face.texture_key);
            // 获取UV坐标
            const Vector2f &uv0 = face.uv_up_left;
            const Vector2f &uv1 = face.uv_down_right;
            bool is_cullface = false;

            // 一个方块的六个面中，每一个面的哪一个顶点可以算作“左上角”呢？
            // 我们总是以“左上角”为起始点，按顺时针方向排列顶点，因此uv总是一样的，而顶点位置具体取哪一个坐标，
            // 则根据方向不同而不同。其中SORTH认为是方块的“正面”（相机默认方向的反方向），UP是上面
            switch (face.direction) {
            case Direction::DOWN: {
                quad.vertices[0] = {Vector3f{min.x, min.y, max.z}, uv0};
                quad.vertices[1] = {Vector3f{max.x, min.y, max.z}, Vector2f{uv1.x, uv0.y}};
                quad.vertices[2] = {Vector3f{max.x, min.y, min.z}, uv1};
                quad.vertices[3] = {Vector3f{min.x, min.y, min.z}, Vector2f{uv0.x, uv1.y}};
                is_cullface = min.y == 0;
                break;
            }
            case Direction::UP: {
                quad.vertices[0] = {Vector3f{min.x, max.y, min.z}, uv0};
                quad.vertices[1] = {Vector3f{max.x, max.y, min.z}, Vector2f{uv1.x, uv0.y}};
                quad.vertices[2] = {Vector3f{max.x, max.y, max.z}, uv1};
                quad.vertices[3] = {Vector3f{min.x, max.y, max.z}, Vector2f{uv0.x, uv1.y}};
                is_cullface = max.y == 1;
                break;
            }
            case Direction::NORTH: {
                quad.vertices[0] = {Vector3f{max.x, max.y, min.z}, uv0};
                quad.vertices[1] = {Vector3f{min.x, max.y, min.z}, Vector2f{uv1.x, uv0.y}};
                quad.vertices[2] = {Vector3f{min.x, min.y, min.z}, uv1};
                quad.vertices[3] = {Vector3f{max.x, min.y, min.z}, Vector2f{uv0.x, uv1.y}};
                is_cullface = min.z == 0;
                break;
            }
            case Direction::SOUTH: {
                quad.vertices[0] = {Vector3f{min.x, max.y, max.z}, uv0};
                quad.vertices[1] = {Vector3f{max.x, max.y, max.z}, Vector2f{uv1.x, uv0.y}};
                quad.vertices[2] = {Vector3f{max.x, min.y, max.z}, uv1};
                quad.vertices[3] = {Vector3f{min.x, min.y, max.z}, Vector2f{uv0.x, uv1.y}};
                is_cullface = max.z == 1;
                break;
            }
            case Direction::WEST: {
                quad.vertices[0] = {Vector3f{min.x, max.y, min.z}, uv0};
                quad.vertices[1] = {Vector3f{min.x, max.y, max.z}, Vector2f{uv1.x, uv0.y}};
                quad.vertices[2] = {Vector3f{min.x, min.y, max.z}, uv1};
                quad.vertices[3] = {Vector3f{min.x, min.y, min.z}, Vector2f{uv0.x, uv1.y}};
                is_cullface = min.x == 0;
                break;
            }
            case Direction::EAST: {
                quad.vertices[0] = {Vector3f{max.x, max.y, max.z}, uv0};
                quad.vertices[1] = {Vector3f{max.x, max.y, min.z}, Vector2f{uv1.x, uv0.y}};
                quad.vertices[2] = {Vector3f{max.x, min.y, min.z}, uv1};
                quad.vertices[3] = {Vector3f{max.x, min.y, max.z}, Vector2f{uv0.x, uv1.y}};
                is_cullface = max.x == 1;
                break;
            }
            }

            // 设置染色索引
            quad.tintindex = face.tintindex;

            if (is_cullface) {
                baked_model.culled_quads[std::to_underlying(face.direction)].emplace_back(quad);
            } else {
                baked_model.unculled_quads.emplace_back(quad);
            }
        }
    }

    return baked_model;
}

} // namespace Craft