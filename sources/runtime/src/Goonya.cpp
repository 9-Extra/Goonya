#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <locale>
#include <runtime/Goonya.h>
#include <sys/types.h>

#include "core/ThreadPool.h"
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
#include "platform/imgui/imgui_module.h"
#include "resource/BuiltinResource.h"
#include "resource/ResMng.h"

namespace Goonya {

static uint32_t calculate_fps(GameClock::ClockType::duration delta) {
    using Clock = std::chrono::steady_clock;
    static Clock::duration average_frame_time = Clock::duration::zero();
    const float smoothing_factor = 0.05f;

    Clock::duration::rep delta_count = std::lerp(average_frame_time.count(), delta.count(), smoothing_factor);
    average_frame_time = Clock::duration(std::max<Clock::duration::rep>(delta_count, 1)); // 防止被0除

    return static_cast<uint32_t>(std::chrono::seconds(1) / average_frame_time);
}

void init_engine() {
    std::locale::global(std::locale("")); // 使用系统指定的本地化
    set_current_thread_name("game");

    LOG_INFO("Running on pwd: {}", std::filesystem::current_path());

    EventBus::initalize();
    Input::initalize();
    Display::initialize(1080, 720);

    current_thread_type = ThreadType::RENDER; // 目前先不拆分线程，资源加载的问题没有解决
    GL.initialize();

    resources.init(u8"../assets/");
    init_buildin_resource();

    renderer.init();

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

    renderer.render();

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
    // 需要保证此函数在没有初始化或者初始化到一半的情况下也能正常工作
    LOG_WARN("退出");

    World::world_list.clear();
    renderer.clear();

    THREAD_POOL.stop_all();
    resources.clear(); // 在设备drop之前清理资源

    GL.drop();
    ImguiMng::drop();

    Display::drop();
    core_logger->flush();
}
} // namespace Goonya
