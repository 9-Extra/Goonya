#pragma once

#include <cstdint>
#include <string>
#include <tuple>

#include "core/enum_operator.h"
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

enum class CursorMode {
    FREE = 0,
    CAPTURED = 1,
    VISIBLE = 0,
    HIDDEN = 2,

    // ------------
    DEFAULT = FREE | VISIBLE,
    DISABLED = CAPTURED | HIDDEN,
};

DECLARE_ENUM_OPERATORS(CursorMode);

class Display {
public:
    static GLFWwindow *window;

public:
    Display() = delete;

    static void initialize(uint32_t width, uint32_t height);
    static void drop();

    static void set_title(const std::string &title);
    static CursorMode get_cursor_mode() noexcept;
    static void set_cursor_mode(CursorMode mode);

    static void poll_events();
    static std::tuple<uint32_t, uint32_t> get_size();
    static void swap();

private:
    static void create_window(uint32_t width, uint32_t height);
};

} // namespace Goonya
