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
#include "function/renderer/Renderer.h"
#include "function/world/World.h"
#include "platform/display/display.h"
#include "platform/imgui/imgui_module.h"

namespace Goonya {

static uint32_t calculate_fps(GameClock::ClockType::duration delta) {
    using Clock = std::chrono::steady_clock;
    static Clock::duration average_frame_time = Clock::duration::zero();
    const float smoothing_factor = 0.05f;

    average_frame_time = Clock::duration(
        (Clock::duration::rep)std::lerp(average_frame_time.count(), delta.count(), smoothing_factor));

    return static_cast<uint32_t>(std::chrono::seconds(1) / average_frame_time);
}

void init_engine() {
    std::locale::global(std::locale("")); // 使用系统指定的本地化
    set_current_thread_name("game");

    LOG_INFO("Running on pwd: {}", std::filesystem::current_path().string());

    EventBus::initalize();
    Input::initalize();
    Display::initialize(1080, 720);

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
    ImGui::ShowDemoWindow();
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
    ImguiMng::drop();

    Display::drop();
    core_logger->flush();
}
} // namespace Goonya