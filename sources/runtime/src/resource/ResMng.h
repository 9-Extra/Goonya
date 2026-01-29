#pragma once

#include "core/as_u8string.h"
#include "core/hash_helper.h"
#include "core/log/Log.h"
#include "resource/Loader.h"
#include "resource/Resource.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <concepts>
#include <filesystem>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace Goonya {
// 资源管理器
class RenderResource final {
private:
    std::filesystem::path resource_dir;

    std::unordered_map<AssetKey, Ref<Resource>, StringHash, StringEqual> storage;
    std::unordered_map<std::string, std::shared_ptr<ResourceLoader>> loaders;

public:
    void init(const std::filesystem::path &asset_dir) {
        this->resource_dir = asset_dir.lexically_normal();
        register_all_loaders();
    }

    void clear() { storage.clear(); }

    const std::filesystem::path &get_root_dir() const noexcept { return resource_dir; }

    bool put_resource(const AssetKey &key, Ref<Resource> res) {
        auto [_, update] = storage.insert_or_assign(key, res);
        return update;
    }

    template <std::derived_from<Resource> T>
    Ref<T> load_resource(std::string_view key) {
        Ref<Resource> res = load_resource(key);
        GN_ASSERT(res);

        // 进行类型检查
        Ref<T> r = Ref<T>::cast_from(res);
        if (!r) {
            throw RuntimeError(
                std::format("实际加载的资源类型{}和目标的类型{}不一致", typeid(res.get()).name(), typeid(T).name()));
        }

        return r;
    }
    Ref<Resource> load_resource(std::string_view key);

    AssetKey path_to_key(const std::filesystem::path &path) const {
        std::string relative_path{as_string_view(std::filesystem::relative(path, resource_dir).u8string())};
        if (relative_path.ends_with(".meta")) {
            relative_path.resize(relative_path.size() - 5);
        }
        return relative_path;
    }

    void register_loader(std::shared_ptr<ResourceLoader> loader) {
        GN_ASSERT(loader);
        for (auto type : loader->supported_types) {
            GN_ASSERT(!type.empty());
            auto [_, inserted] = loaders.emplace(type, loader);
            if (!inserted) {
                LOG_ERROR("类型{}的加载器重复注册", type);
            }
        }
    }

private:
    void scan();
    Ref<Resource> try_load(const std::filesystem::path &path);
};

extern RenderResource resources;
} // namespace Goonya