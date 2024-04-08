#pragma once

#include <cstdint>
#include <string>
#include <tuple>

#include "core/input/input.h"

namespace Goonya {

namespace Display{

namespace Events {
struct SysWindowClose{};
struct SysWindowDeActive{};
struct SysRawMouseMove{
    int32_t x, y;
};
struct SysMousePos{
    int32_t x, y;
};
struct SysMouseClick{
    Input::MOUSEKEY key;
    bool up_down;
};
struct SysKeyEvent{
    Input::KeyCode key;
    bool up_down;
};
}

    void initalize(uint32_t width, uint32_t height);
    void drop();

    void init_for_imgui();

    void set_title(const std::string& title);
    void poll_events();
    std::tuple<uint32_t, uint32_t> get_size();
    void swap();
}

}