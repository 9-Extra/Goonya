#include "World.h"
#include "core/ThreadPool.h"
#include "function/renderer/Renderer.h"

#include <imgui.h>

namespace Goonya {

std::forward_list<World> World::world_list;

void TickFunction::register_ticker(World *world) noexcept {
    GN_ASSERT(world != nullptr);
    if (world == owner_world) {
        return;
    }
    unregister_ticker();
    world->register_ticker(this);
}

void TickFunction::unregister_ticker() noexcept {
    if (is_registered()) {
        owner_world->unregister_ticker(this);
    }
}

TickFunction::~TickFunction() { unregister_ticker(); }

// -------------------------World-------------------------------
World::World() : scene(renderer.create_scene()) {
    root = std::make_shared<GObject>("__root__");
    root->set_world(this);
}
World::~World() {
    root->set_world(nullptr);
    root.reset();
    tick_count = 0;
    renderer.drop_scene(scene);
}

void World::tick() {
    tick_count++;

    GN_ASSERT(root && root->get_world() == this);

    // here
    if (ImGui::Begin("SceneTree")) {
        ImGui::TextUnformatted("World");
        draw_scene_tree_node(root);
    }
    ImGui::End();

    for (const auto &t : tick_functions) {
        t->tick();
    }

    for (auto &&obj : deferred_update_list) {
        if (auto p = obj.lock(); p) {
            p->do_deferred_update();
        }
    }
    deferred_update_list.clear();

    main_thread_process();
}

void World::fixed_tick() {
    for (const auto &t : fixed_tick_functions) {
        t->tick();
    }
}

void World::register_ticker(TickFunction *function) {
    GN_ASSERT(function && !function->TickFunction::is_registered());
    switch (function->get_tick_type()) {
    case TickType::TICK:
        tick_functions.emplace(function);
        break;
    case TickType::FIXED_TICK:
        fixed_tick_functions.emplace(function);
        break;
    }
    function->owner_world = this;
}
void World::unregister_ticker(TickFunction *function) {
    GN_ASSERT(function && function->TickFunction::is_registered());
    switch (function->get_tick_type()) {
    case TickType::TICK:
        tick_functions.erase(function);
        break;
    case TickType::FIXED_TICK:
        fixed_tick_functions.erase(function);
        break;
    }
    function->owner_world = nullptr;
}

void World::draw_scene_tree_node(const std::shared_ptr<GObject> &node) const noexcept {
    if (!node) {
        return;
    }

    const auto &children = node->get_children();
    const bool is_leaf = children.empty();

    // 使用节点指针作为唯一ID，避免兄弟节点重名导致的ImGui ID冲突
    ImGui::PushID(node.get());

    // 设置树节点标志
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (is_leaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    // 获取节点名称
    std::string node_name = std::string(node->get_name());

    // 绘制树节点
    bool is_open = ImGui::TreeNodeEx(node_name.c_str(), flags);

    if (is_open) {
        // 递归绘制子节点
        for (const auto &child : children) {
            draw_scene_tree_node(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

} // namespace Goonya