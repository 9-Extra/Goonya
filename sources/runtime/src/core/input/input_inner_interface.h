#pragma once

#include "input.h"

namespace Goonya {
namespace Input {

namespace Detail {

extern bool keys_state_last_tick[MAX_KEYCODE];
extern bool keys_state[MAX_KEYCODE];
extern int32_t mouse_delta_x;
extern int32_t mouse_delta_y;
extern int32_t mouse_pos_x;
extern int32_t mouse_pos_y;

extern bool mouse_key_state_last_tick[MOUSEKEY::MOUSE_KEY_MAX];
extern bool mouse_key_state[MOUSEKEY::MOUSE_KEY_MAX];

inline void on_key_down(KeyCode key) {
    keys_state[key] = true;
}

inline void on_key_up(KeyCode key) {
    keys_state[key] = false;
}

inline void on_mouse_move(int32_t x, int32_t y) {
    mouse_delta_x += x;
    mouse_delta_y += y;
}

inline void on_mouse_set(int32_t x, int32_t y) {
    mouse_pos_x = x;
    mouse_pos_y = y;
}

inline void tick_update_clear(){
    for (size_t i = 0; i < MAX_KEYCODE; i++) {
        keys_state_last_tick[i] = keys_state[i];
    }
    mouse_delta_x = 0;
    mouse_delta_y = 0;
}

} // namespace Detail
} // namespace Input
} // namespace Goonya