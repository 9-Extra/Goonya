#pragma once

#include "GObject.h"
#include "function/world/Component.h"

#include <forward_list>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Goonya {

class World;

enum class TickType {
    TICK,
    FIXED_TICK,
};

struct TickFunction {
private:
    friend class World;
    World *owner_world = nullptr;
    TickType tick_type = TickType::TICK;

public:
    TickFunction() = default;
    explicit TickFunction(TickType type) : tick_type(type) {}
    virtual ~TickFunction();

    TickType get_tick_type() const noexcept { return tick_type; }
    bool is_registered() const noexcept { return owner_world != nullptr; }
    void register_ticker(World *world) noexcept;
    void unregister_ticker() noexcept;

    virtual void tick() = 0;
};

class RScene;

class World final {
public:
    static std::forward_list<World> world_list;

private:
    std::shared_ptr<GObject> root;
    uint64_t tick_count = 0;
    RScene *scene;

    std::unordered_set<TickFunction *> tick_functions;
    std::unordered_set<TickFunction *> fixed_tick_functions;
    std::vector<std::weak_ptr<GObject>> deferred_update_list;

public:
    World();
    ~World();

public:
    static World *create_world() { return &world_list.emplace_front(); }
    static void delete_world(World *world) {
        world_list.remove_if([world](const World &w) { return &w == world; });
    }
    World(const World &) = delete;
    // --------------------管理--------------------------
    RScene *get_scene() const noexcept { return scene; }

    std::shared_ptr<GObject> get_root() const noexcept { return root; }

    std::shared_ptr<GObject> set_root(const std::shared_ptr<GObject> &new_root) noexcept {
        root->set_world(nullptr);
        new_root->set_world(this);
        return std::exchange(root, new_root);
    }

    // ---------------------Tick--------------------------
    uint64_t get_tick_count() const { return tick_count; }

    void tick();
    void fixed_tick();

    void register_ticker(TickFunction *function);
    void unregister_ticker(TickFunction *function);

    // ---------------------延迟更新---------------------
    void add_deferred_update(const std::weak_ptr<GObject> &obj) noexcept { deferred_update_list.emplace_back(obj); }
    void remove_deferred_update(const std::weak_ptr<GObject> &obj) noexcept {
        // deferred_update_list中所有的GObject都应该是alive的
        auto to_remove = obj.lock();
        GN_ASSERT(to_remove);
        auto iter = std::ranges::find_if(deferred_update_list, [to_remove](const auto &rhs) {
            return to_remove == rhs.lock(); // 判断两个weak_ptr相等
        });
        GN_ASSERT(iter != deferred_update_list.end());
        deferred_update_list.erase(iter);
    }

private:
    void draw_scene_tree_node(const std::shared_ptr<GObject> &node) const noexcept;
};
} // namespace Goonya
