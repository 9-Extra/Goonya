#pragma once

#include <cstdint>
#include <string>
#include <tuple>

#include "core/input/input.h"

struct GLFWwindow;

namespace Goonya {

namespace Events {
struct SysWindowClose {};
struct SysWindowDeActive {};
struct SysMousePos {
    double x, y;
};
struct SysMouseClick {
    Input::MouseKey key;
    Input::KeyState state;
};
struct SysKeyEvent {
    Input::KeyCode key;
    Input::KeyState state;
};
struct SysDisplayResize {
    std::tuple<uint32_t, uint32_t> size;
};
} // namespace Events

class Display {
public:
    static GLFWwindow *window;

    Display() = delete;

    static void initialize(uint32_t width, uint32_t height);
    static void drop();

    static void set_title(const std::string &title);
    static void poll_events();
    static std::tuple<uint32_t, uint32_t> get_size();
    static void swap();

private:
    static void create_window(uint32_t width, uint32_t height);
};

} // namespace Goonya
