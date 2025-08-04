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

class Resource;

struct ResourceClassInfo {
    Resource *(*creator)(); // 无参构造函数，调用后返回Resource*
};

// 抄Godot的ClassDB，但现在只管Resource
// 所有成员都是静态的
class ResourceClassDB final {
private:
    static std::unordered_map<std::string, ResourceClassInfo> class_infos;

public:
    template <class T>
    static void register_class() noexcept {
        class_infos.emplace(T::get_class_name_static(), ResourceClassInfo{[]() { return T::T(); }});
    }

    static bool is_class_registered(const std::string &class_name) noexcept { return class_infos.contains(class_name); }

    static const std::unordered_map<std::string, ResourceClassInfo> &get_class_info() noexcept { return class_infos; }
};

#define DEFINE_CLASS(class_name, desc_type)                                                                            \
    friend class ::Goonya::Resource::ResourceClassDB;                                                                  \
    static const std::string_view get_class_name_static() noexcept { return #class_name; }                             \
    virtual std::string_view get_class_name() const noexcept { return #class_name; }                                   \
    using DescType = desc_type;                                                                                                             \
private:

// 所有资源继承此类
class Resource : public intrusive_ptr_base<Resource> {
    DEFINE_CLASS(Resource, int);

private:
    std::string name; // 单纯为了看，没有查找等用途

public:
    Resource() = default;
    virtual ~Resource() = default;

    const std::string &get_name() const noexcept { return name; }
    void set_name(std::string name) noexcept { this->name = std::move(name); }


};

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
