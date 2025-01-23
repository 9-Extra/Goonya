#include <cmath>
#include <cstdint>
#include <format>
#include <imgui.h>
#include <runtime/Goonya.h>

#include "function/renderer/RenderResource.h"
#include "function/renderer/Renderer.h"
#include "platform/display/display.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "platform/graphics/graphics.h"
#include "platform/imgui/imgui_module.h"
#include "core/input/input.h"
#include "core/timer/timer.h"
#include "core/world/World.h"
#include "runtime/log/Log.h"

namespace Goonya {

static uint32_t calculate_fps(float delta_time) {
    const float ratio = 0.05f; // 平滑比例
    static float avarage_frame_time = std::nanf("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

void init_engine() {
    logger.inititalize();

    EventBus::initalize();
    Input::initalize();
    Timer::initialize();
    Display::initalize(1080, 720);
    Graphics::initialize(Graphics::GraphicsAPIType::OPENGL);
    Graphics::renderer.init();

    ImguiMng::init();

    EventBus::subscribe_event<Events::PostTick, void>(0, nullptr, [](void *, Events::PostTick &e) {
        uint32_t fps = calculate_fps(Timer::delta());
        Display::set_title(std::format("Goonya - FPS: {}", fps));
        return false;
    });

    EventBus::subscribe_event<Display::Events::SysWindowClose, void>(0, nullptr,
        [](void *, Display::Events::SysWindowClose &e) {
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
    EventBus::ListenerID id = EventBus::subscribe_event<Events::EngineStop, bool>(
        10, &should_continue, [](bool *should_continue, Events::EngineStop &e) {
            *should_continue = false;
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

    EventBus::remove_listener<Display::Events::SysWindowClose>(id);
}

void drop_engine() {
    LOG_WARN("退出");

    world.clear();
    Graphics::renderer.clear();
    Graphics::resources.clear(); // 在设备drop之前清理资源
    ImguiMng::drop();

    Graphics::drop();
    Display::drop();
    Timer::drop();

    logger.drop();
}
} // namespace Goonya