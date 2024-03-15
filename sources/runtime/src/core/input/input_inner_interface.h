#pragma once

#include "input.h"

namespace Goonya {
namespace Input {
namespace Detail {

extern bool keys_state[MAX_KEYCODE];
extern bool tick_key_down[MAX_KEYCODE];
extern bool tick_key_up[MAX_KEYCODE];
extern int32_t mouse_delta_x;
extern int32_t mouse_delta_y;
extern int32_t mouse_pos_x;
extern int32_t mouse_pos_y;

inline void on_key_down(KeyCode key) {
    keys_state[key] = true;
    tick_key_down[key] = true;
}

inline void on_key_up(KeyCode key) {
    keys_state[key] = false;
    tick_key_up[key] = true;
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
        tick_key_down[i] = false;
        tick_key_up[i] = false;
    }
    mouse_delta_x = 0;
    mouse_delta_y = 0;
}

} // namespace Detail
} // namespace Input
} // namespace Goonya