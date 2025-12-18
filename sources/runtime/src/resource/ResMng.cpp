#include "ResMng.h"

#include "json/reader.h"
#include "json/value.h"
#include <cassert>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>

#include "core/RefCount.h"
#include "core/as_u8string.h"
#include "core/format_exception.h"
#include "core/log/Log.h"
#include "core/path_formatter.h"
#include "resource/Loader.h"
#include "resource/Resource.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

RenderResource resources; // Global

namespace fs = std::filesystem;

void RenderResource::scan() {
    assert(!resource_dir.empty());
    // for (const fs::directory_entry &dir_entry :
    //      fs::recursive_directory_iterator(resource_dir, fs::directory_options::follow_directory_symlink)) {
    //     if (dir_entry.is_regular_file()) {
    //         const fs::path &path = dir_entry.path();
    //         std::u8string ext = path.extension().u8string();
    //         if (ext == u8".meta") {
    //             try {
    //                 try_load(path);
    //             } catch (const std::exception &e) {
    //                 LOG_WARN("加载资源{}时遇到错误：{}\n{}", path.generic_string(), e.what(), format_exception(e));
    //             }
    //         }
    //     }
    // }
}

Ref<Resource> RenderResource::load_resource(std::string_view key) {
    if (auto iter = storage.find(key); iter != storage.end()) {
        return iter->second;
    }

    Ref<Resource> res = {};
    size_t split = key.find(':');
    if (split != std::string::npos) {
        // 读取包内资源
        std::string_view pack_key = key.substr(0, split);
        Ref<ResourcePack> pack = load_resource<ResourcePack>(pack_key);
        if (!pack) {
            LOG_ERROR("加载资源包{}时出错", pack_key);
            return res;
        }

        std::string_view inner_key = key.substr(split + 1);
        auto iter = pack->contents.find(inner_key);
        if (iter == pack->contents.end()) {
            LOG_ERROR("资源包{}内不存在资源{}", pack_key, inner_key);
            return res;
        }
        res = iter->second;
        storage.emplace(key, res); // 添加直接使用key访问的捷径
    } else {
        // 普通资源
        try {
            LOG_TRACE("加载资源{}", key);
            res = try_load(resource_dir / as_u8string_view(std::format("{}.meta", key)));
            assert(res);
            storage.emplace(key, res);
        } catch (const std::exception &e) {
            LOG_ERROR("加载资源{}时出现异常{}", key, format_exception(e));
        }
    }

    return res;
}

Ref<Resource> RenderResource::try_load(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        throw RuntimeError(std::format("打开文件\"{}\"失败", path));
    }

    Json::Value meta;
    if (!Json::parseFromStream(Json::CharReaderBuilder(), file, &meta, nullptr)) {
        throw RuntimeError("解析Json出错");
    }
    const AssetKey &key = path_to_key(path);
    if (key.empty()) {
        throw RuntimeError("键未能正常生成");
    }

    std::filesystem::path base_dir = path.parent_path();
    const std::string &res_type = meta["type"].asString();

    auto iter = loaders.find(res_type);
    if (iter == loaders.end()) {
        throw RuntimeError("用于类型\"{}\"资源的加载器不存在");
    }

    ResourceLoader *loader = iter->second.get();
    Ref<Resource> res = loader->load(res_type, base_dir, meta["name"].asString(), meta["content"]);

    if (!res) {
        // 加载器出错时应该抛异常
        throw RuntimeError("未知原因");
    }

    return res;
}
} // namespace Goonya