#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <locale>
#include <runtime/Goonya.h>

#include "core/ThreadUtils.h"
#include "core/clock/GameClock.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/input/input.h"
#include "core/log/Log.h"
#include "core/path_formatter.h"
#include "function/renderer/Renderer.h"
#include "function/world/World.h"
#include "platform/display/display.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Mesh.h"
#include "platform/imgui/imgui_module.h"
#include "resource/HardcodeAssets.h"
#include "resource/ResMng.h"

namespace Goonya {

static uint32_t calculate_fps(GameClock::ClockType::duration delta) {
    using Clock = std::chrono::steady_clock;
    static Clock::duration average_frame_time = Clock::duration::zero();
    const float smoothing_factor = 0.05f;

    average_frame_time =
        Clock::duration((Clock::duration::rep)std::lerp(average_frame_time.count(), delta.count(), smoothing_factor));

    return static_cast<uint32_t>(std::chrono::seconds(1) / average_frame_time);
}

static void init_buildin_resource() {
    // 部分硬编码的mesh
    std::span<const std::byte> plane_vertex_span = std::as_bytes(std::span(Assets::plane_vertices));
    Ref<Graphics::Mesh> plane = Graphics::graphics_api->create_mesh(Assets::plane_vertices_vertex_layout);
    plane->set_vertices(0, plane_vertex_span);
    plane->set_indices(Assets::plane_indices);
    plane->submeshes.emplace_back(Graphics::SubMesh{.start_index = 0,
                                                    .index_count = (uint32_t)Assets::plane_indices.size(),
                                                    .base_vertex_offset = 0,
                                                    .topology = Graphics::Topology::TRIANGLE,
                                                    .aabb = Assets::plane_aabb});
    resources.put_resource("plane", plane);

    // 添加天空盒的mesh，因为格式不一样所以单独处理
    std::span<const std::byte> skybox_cube_vertex_span = std::as_bytes(std::span(Assets::skybox_cube_vertices));
    Ref<Graphics::Mesh> skybox_cube = Graphics::graphics_api->create_mesh(Assets::skybox_cube_vertex_layout);
    skybox_cube->set_vertices(0, skybox_cube_vertex_span);
    skybox_cube->set_indices(Assets::skybox_cube_indices);
    skybox_cube->submeshes.emplace_back(Graphics::SubMesh{.start_index = 0,
                                                    .index_count = (uint32_t)Assets::skybox_cube_indices.size(),
                                                    .base_vertex_offset = 0,
                                                    .topology = Graphics::Topology::TRIANGLE,
                                                    .aabb = Assets::skybox_cube_aabb});
    resources.put_resource("skybox_cube", skybox_cube);
}

void init_engine() {
    std::locale::global(std::locale("")); // 使用系统指定的本地化
    set_current_thread_name("game");

    LOG_INFO("Running on pwd: {}", std::filesystem::current_path());

    EventBus::initalize();
    Input::initalize();
    Display::initialize(1080, 720);

    current_thread_type = ThreadType::RENDER; // 目前先不拆分线程，资源加载的问题没有解决
    Graphics::initialize(Graphics::GraphicsAPIType::OPENGL);

    resources.init(u8"../assets/");
    init_buildin_resource();

    Graphics::renderer.init();

    ImguiMng::init();

    EventBus::subscribe_event<Events::PostTick>(0, [](Events::PostTick &e) {
        uint32_t fps = calculate_fps(GAME_CLOCK.delta());
        Display::set_title(std::format("Goonya - FPS: {}", fps));
        return false;
    });

    EventBus::subscribe_event<Events::SysWindowClose>(0, [](Events::SysWindowClose &e) {
        EventBus::dispatch_event(Events::EngineStop{});
        return false;
    });
}

static void logic_tick() {
    ImGui::ShowDemoWindow();
    // auto [x, y] = Input::get_mouse_pos();
    // LOG_DEBUG("鼠标位置: {}, {}", x, y);
    for (auto &&w : World::world_list) {
        w.tick();
    }
}

static void fixed_tick() {
    // auto [x, y] = Input::get_mouse_pos();
    // LOG_DEBUG("鼠标位置: {}, {}", x, y);
    for (auto &&w : World::world_list) {
        w.fixed_tick();
    }
}

static void render_frame() {

    Graphics::renderer.render();

    ImGui::EndFrame();
    ImGui::Render();
    ImguiMng::render();
}

void main_loop() {
    bool should_continue = true;
    EventBus::ListenerID id =
        EventBus::subscribe_event<Events::EngineStop>(10, [&should_continue](Events::EngineStop &e) {
            should_continue = false;
            return true;
        });

    GAME_CLOCK.start();

    while (should_continue) {
        GAME_CLOCK.update();

        ImguiMng::new_frame();
        ImGui::NewFrame();

        Display::poll_events();
        while (GAME_CLOCK.advance_fixed_tick()) {
            fixed_tick();
        }

        logic_tick();
        render_frame();

        EventBus::dispatch_event(Events::PostTick{});
        Display::swap();
    }

    EventBus::remove_listener<Events::SysWindowClose>(id);
}

void drop_engine() {
    LOG_WARN("退出");

    World::world_list.clear();
    Graphics::renderer.clear();
    resources.clear(); // 在设备drop之前清理资源
    Graphics::drop();
    ImguiMng::drop();

    Display::drop();
    core_logger->flush();
}
} // namespace Goonya