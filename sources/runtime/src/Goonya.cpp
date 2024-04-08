#include <Windows.h>
#include <cmath>
#include <cstdint>
#include <format>
#include <imgui.h>
#include <runtime/Goonya.h>


#include "core/display/display.h"
#include "core/event/event.h"
#include "core/graphics/graphics.h"
#include "core/imgui/imgui_module.h"
#include "core/input/input.h"
#include "core/timer/timer.h"
#include "core/world/World.h"
#include "runtime/log/Log.h"
#include "core/events.h"


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

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    Event::initalize();
    Input::initalize();
    Timer::initialize();
    Display::initalize(1080, 720);
    Graphics::initialize();

    ImguiMng::init();

    Event::subscribe_event<Events::PostTick, void>(0, nullptr, [](void*, Events::PostTick& e){
        uint32_t fps = calculate_fps(Timer::delta());
        Display::set_title(std::format("Goonya - FPS: {}", fps));
        return false;
    });

    load_scene_from_json("../assets/scene2.json"); // 整个场景的所有物体都从json加载了
}


void logic_tick() {
    ImGui::ShowDemoWindow();

    // auto [x, y] = Input::get_mouse_pos();
    // LOG_DEBUG("鼠标位置: {}, {}", x, y);
    world.tick();
}

void render_frame() {

    Graphics::render();

    ImGui::EndFrame();
    ImGui::Render();
    ImguiMng::render();
}

void main_loop() {
    bool should_continue = true;
    Event::ListenerID id = Event::subscribe_event<Display::Events::SysWindowClose, bool>(
        10, &should_continue, [](bool *should_continue, Display::Events::SysWindowClose& e) {
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

        Event::dispatch_event(Events::PostTick{Timer::delta()});
    }

    Event::remove_listener<Display::Events::SysWindowClose>(id);
}

void drop_engine() {
    LOG_WARN("退出");

    ImguiMng::drop();

    Graphics::drop();
    Display::drop();
    Timer::drop();

    logger.drop();
}
} // namespace Goonya