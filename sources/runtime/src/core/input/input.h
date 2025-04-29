#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>

namespace Goonya::Input {

enum class KeyState : uint8_t { UP, DOWN };

inline KeyState revert(KeyState state) { return state == KeyState::UP ? KeyState::DOWN : KeyState::UP; }

enum class KeyCode : uint32_t {
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
KeyState get_key_state(KeyCode key);
inline KeyState get_key_state(char key) { return get_key_state((KeyCode)key); }

bool is_key_click(KeyCode key);
inline bool is_key_click(char key) { return is_key_click((KeyCode)key); }
bool is_key_release(KeyCode key);
inline bool is_key_release(char key) { return is_key_release((KeyCode)key); }

std::tuple<int32_t, int32_t> get_mouse_move();
std::tuple<int32_t, int32_t> get_mouse_pos();

enum MouseKey {
    LEFT,
    RIGHT,
    MIDDLE,

    MOUSE_KEY_MAX
};

KeyState get_mouse_state(MouseKey key);
bool is_mouse_click(MouseKey key);
bool is_mouse_release(MouseKey key);

} // namespace Goonya::Input
