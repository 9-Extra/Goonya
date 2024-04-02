#include "input.h"
#include <cstdint>

namespace Goonya {
namespace Input {

namespace Detail {

bool keys_state_last_tick[MAX_KEYCODE];
bool keys_state[MAX_KEYCODE];
int32_t mouse_delta_x;
int32_t mouse_delta_y;
int32_t mouse_pos_x;
int32_t mouse_pos_y;

bool mouse_key_state_last_tick[MOUSEKEY::MOUSE_KEY_MAX];
bool mouse_key_state[MOUSEKEY::MOUSE_KEY_MAX];

} // namespace Detail

void reset_state() {
    for (size_t i = 0; i < MAX_KEYCODE; i++) {
        Detail::keys_state[i] = false;
        Detail::keys_state_last_tick[i] = false;
    }

    for (size_t i = 0; i < MOUSEKEY::MOUSE_KEY_MAX; i++) {
        Detail::mouse_key_state[i] = false;
        Detail::mouse_key_state_last_tick[i] = false;
    }

    Detail::mouse_delta_x = 0;
    Detail::mouse_delta_y = 0;
    Detail::mouse_pos_x = 0;
    Detail::mouse_pos_y = 0;
}

bool get_key_state(KeyCode key) { return Detail::keys_state[key]; }

bool is_key_down(KeyCode key) { return !Detail::keys_state_last_tick[key] && Detail::keys_state[key]; }

bool is_key_up(KeyCode key) { return Detail::keys_state_last_tick[key] && !Detail::keys_state[key]; }

std::tuple<int32_t, int32_t> get_mouse_move() { return std::make_tuple(Detail::mouse_delta_x, Detail::mouse_delta_y); }

std::tuple<int32_t, int32_t> get_mouse_pos() {
    return std::make_tuple(Detail::mouse_pos_x, Detail::mouse_pos_y);
}

bool get_mouse_state(MOUSEKEY key){
    return Detail::mouse_key_state[key];
}
bool is_mouse_down(MOUSEKEY key){
    return Detail::mouse_key_state[key] && !Detail::mouse_key_state_last_tick[key];
}
bool is_mouse_up(MOUSEKEY key){
    return !Detail::mouse_key_state[key] && Detail::mouse_key_state_last_tick[key];
}

} // namespace Input
} // namespace Goonya