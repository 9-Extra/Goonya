#pragma once

#include <cstdint>
#include <limits>
#include <tuple>

namespace Goonya {
namespace Input {

enum class KeyCode: uint32_t{
    LSHIFT,
    RSHIFT,
    LCTRL,
    RCTRL,
    LALT,
    RALT,

    ENTER,
    ESCAPE,
    TAB,
    SPACE,

    UNKNOWN = std::numeric_limits<uint32_t>::max(),
};

const size_t MAX_KEYCODE = 256;

void initalize();
void reset_state();
bool get_key_state(KeyCode key);
inline bool get_key_state(char key){
    return get_key_state((KeyCode)key);
}

bool is_key_down(KeyCode key);
inline bool is_key_down(char key){
    return is_key_down((KeyCode)key);
}
bool is_key_up(KeyCode key);
inline bool is_key_up(char key){
    return is_key_up((KeyCode)key);
}

std::tuple<int32_t, int32_t> get_mouse_move();
std::tuple<int32_t, int32_t> get_mouse_pos();

enum MOUSEKEY {
    LEFT,
    RIGHT,
    MIDDLE,

    MOUSE_KEY_MAX
};

bool get_mouse_state(MOUSEKEY key);
bool is_mouse_down(MOUSEKEY key);
bool is_mouse_up(MOUSEKEY key);

} // namespace Input
} // namespace Goonya