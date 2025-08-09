#pragma once

#include <cassert>
#include <json/config.h>
#include <json/json.h>
#include <json/value.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "core/assets.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include "runtime/GoonyaException.h"

namespace Goonya::Resource {

template <class TDesc, class TAsset>
class ResourceContainer {
protected:
    std::string name;
    std::unordered_map<AssetKey, std::tuple<intrusive_ptr<TAsset>, TDesc>> container;

public:
    virtual ~ResourceContainer() = default;
    explicit ResourceContainer(std::string name) : name(std::move(name)) {}
    template <typename T>
        requires std::is_convertible_v<T, TDesc>
    void add(const AssetKey &key, T &&desc) {
        auto [_, ok] = container.emplace(key, std::tuple<intrusive_ptr<TAsset>, TDesc>{nullptr, std::forward<T>(desc)});
        if (!ok) {
            throw RuntimeError(std::format("资源\"{}\"重复注册", key));
        }
    }
    bool contains(const AssetKey &key) const { return container.contains(key); }

    intrusive_ptr<TAsset> get(const AssetKey &key) {
        if (auto iter = container.find(key); iter != container.end()) {
            auto &[asset, desc] = iter->second;
            if (!asset) [[unlikely]] {
                LOG_TRACE("正在加载{}：\"{}\"", name, key);
                asset = load(desc);
            }
            return asset;
        } else {
            throw RuntimeError(std::format("{}\"{}\"未注册", name, key));
        }
    }

    void clear() { container.clear(); }

protected:
    virtual intrusive_ptr<TAsset> load(const TDesc &desc) const = 0;
};

} // namespace Goonya::Resource
