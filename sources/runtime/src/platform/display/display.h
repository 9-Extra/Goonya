#pragma once

#include <cstdint>
#include <string>
#include <tuple>

#include "core/input/input.h"

struct GLFWwindow;

namespace Goonya {

namespace Display{

namespace Events {
struct SysWindowClose{};
struct SysWindowDeActive{};
struct SysMousePos{
    double x, y;
};
struct SysMouseClick{
    Input::MouseKey key;
    Input::KeyState state;
};
struct SysKeyEvent{
    Input::KeyCode key;
    Input::KeyState state;
};
struct SysDisplayResize{
    std::tuple<uint32_t, uint32_t> size;
};
}

extern GLFWwindow *window;

void initalize(uint32_t width, uint32_t height);
void drop();

void set_title(const std::string& title);
void poll_events();
std::tuple<uint32_t, uint32_t> get_size();
void swap();

}

}