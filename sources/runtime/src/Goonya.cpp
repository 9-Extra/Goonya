#include <cmath>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <locale>
#include <runtime/Goonya.h>

#include "core/ThreadUtils.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/input/input.h"
#include "core/log/Log.h"
#include "core/timer/timer.h"
#include "function/world/World.h"
#include "function/renderer/Renderer.h"
#include "platform/display/display.h"
#include "platform/imgui/imgui_module.h"


namespace Goonya {

static uint32_t calculate_fps(float delta_time) {
    static float average_frame_time = std::nanf("");

    if (std::isnormal(average_frame_time)) {
        const float ratio = 0.05f;
        average_frame_time = average_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        average_frame_time = delta_time;
    }

    return static_cast<unsigned int>(1000.0f / average_frame_time);
}

void init_engine() {
    std::locale::global(std::locale("")); // 使用系统指定的本地化
    set_current_thread_name("game");

    LOG_INFO("Running on pwd: {}", std::filesystem::current_path().string());

    EventBus::initalize();
    Input::initalize();
    Timer::initialize();
    Display::initialize(1080, 720);
    
    Graphics::renderer.init();

    ImguiMng::init();

    EventBus::subscribe_event<Events::PostTick>(0, [](Events::PostTick &e) {
        uint32_t fps = calculate_fps(Timer::delta());
        Display::set_title(std::format("Goonya - FPS: {}", fps));
        return false;
    });

    EventBus::subscribe_event<Events::SysWindowClose>(0, [](Events::SysWindowClose &e) {
        EventBus::dispatch_event(Events::EngineStop{});
        return false;
    });
}

void logic_tick() {
    ImGui::ShowDemoWindow();

    // auto [x, y] = Input::get_mouse_pos();
    // LOG_DEBUG("鼠标位置: {}, {}", x, y);
    world.tick();
}

void render_frame() {

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

    while (should_continue) {
        ImguiMng::new_frame();
        ImGui::NewFrame();

        Display::poll_events();
        Timer::tick_update();

        logic_tick();
        render_frame();

        Display::swap();

        EventBus::dispatch_event(Events::PostTick{});
    }

    EventBus::remove_listener<Events::SysWindowClose>(id);
}

void drop_engine() {
    LOG_WARN("退出");

    world.reset();
    Graphics::renderer.clear();
    resources.clear(); // 在设备drop之前清理资源
    ImguiMng::drop();

    Display::drop();
    Timer::drop();

    core_logger->flush();
}
} // namespace Goonya