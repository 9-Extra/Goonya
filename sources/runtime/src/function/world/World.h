#pragma once

#include "GObject.h"
#include "function/renderer/RenderScene.h"
#include "function/renderer/Renderer.h"
#include "function/world/Component.h"
#include "platform/graphics/Graphics.h"
#include <cassert>
#include <forward_list>
#include <memory>
#include <utility>
#include <vector>

namespace Goonya {

struct TickFunction {
    TickFunction() = default;
    virtual ~TickFunction() = default;

    virtual void tick() = 0;
};

class World final {
public:
    static std::forward_list<World> world_list;

private:
    std::shared_ptr<GObject> root;
    uint64_t tick_count = 0;
    Graphics::RenderScene *_main_scene = nullptr;

    std::unordered_set<TickFunction *> tick_functions;
    std::vector<std::weak_ptr<GObject>> deferred_update_list;

public:
    World() {
        _main_scene = Graphics::renderer.create_scene();
        root = std::make_shared<GObject>("__root__");
        root->set_world(this);
    }
    ~World() {
        root.reset();
        tick_count = 0;
        Graphics::enqueue_render_task([scene = _main_scene] { Graphics::renderer.drop_scene(scene); });
    }

public:
    static World *create_world() { return &world_list.emplace_front(); }
    static void delete_world(World *world) {
        world_list.remove_if([world](const World &w) { return &w == world; });
    }
    World(const World &) = delete;
    // --------------------管理--------------------------
    Graphics::RenderScene *main_scene() noexcept {
        assert(_main_scene != nullptr);
        return _main_scene;
    }

    std::shared_ptr<GObject> get_root() const noexcept { return root; }

    std::shared_ptr<GObject> set_root(const std::shared_ptr<GObject> &new_root) noexcept {
        new_root->set_world(this);
        return std::exchange(root, new_root);
    }

    // ---------------------Tick--------------------------
    uint64_t get_tick_count() const { return tick_count; }

    void tick();

    void register_ticker(TickFunction *function) { tick_functions.emplace(function); }
    void unregister_ticker(TickFunction *function) { tick_functions.erase(function); }

    // ---------------------延迟更新---------------------
    void add_deferred_update(const std::weak_ptr<GObject> &obj) noexcept { deferred_update_list.emplace_back(obj); }
    void remove_deferred_update(const std::weak_ptr<GObject> &obj) noexcept {
        auto iter = std::ranges::find_if(deferred_update_list, [obj](const auto &rhs) {
            return !obj.owner_before(rhs) && !rhs.owner_before(obj); // 判断两个weak_ptr相等
        });
        assert(iter != deferred_update_list.end());
        deferred_update_list.erase(iter);
    }
};
} // namespace Goonya