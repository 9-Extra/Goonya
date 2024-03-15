#include <cmath>
#include <cstdint>
#include <format>
#include <runtime/Goonya.h>

#include "core/display/display.h"
#include "core/graphics/graphics.h"
#include "core/input/input.h"
#include "core/input/input_inner_interface.h"
#include "core/timer/timer.h"
#include "runtime/log/Log.h"
#include "EventCallback.h"

namespace Goonya {

void init_engine(){
    logger.inititalize();

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    Input::reset_state();
    Timer::initialize();
    Display::initalize(1080, 720);
    Graphics::initialize();

    should_exit = false;
}

uint32_t calculate_fps(float delta_time) {
    const float ratio = 0.1f; // 平滑比例
    static float avarage_frame_time = std::nanf("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

void logic_tick(){
    auto [x, y] = Input::get_mouse_pos();
    LOG_DEBUG("鼠标位置: {}, {}", x, y);
}

void render_frame(){

}

void main_loop(){
    while (!should_exit){
        Display::poll_events();
        Timer::tick_update();

        logic_tick();
        render_frame();

        Input::Detail::tick_update_clear();
        Graphics::swap();

        uint32_t fps = calculate_fps(Timer::delta());
        Display::set_title(std::format("Goonya - FPS: {}", fps));
    }
}

void drop_engine(){
    LOG_WARN("退出");

    Graphics::drop();
    Display::drop();
    Timer::drop();

    logger.drop();
}
} // namespace Goonya