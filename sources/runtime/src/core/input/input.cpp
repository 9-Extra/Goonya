#include "input.h"
#include <cstdint>

namespace Goonya {
namespace Input {

namespace Detail {

bool keys_state[MAX_KEYCODE];
bool tick_key_down[MAX_KEYCODE];
bool tick_key_up[MAX_KEYCODE];
int32_t mouse_delta_x;
int32_t mouse_delta_y;
int32_t mouse_pos_x;
int32_t mouse_pos_y;

} // namespace Detail

void reset_state() {
    for (size_t i = 0; i < MAX_KEYCODE; i++) {
        Detail::keys_state[i] = false;
        Detail::tick_key_down[i] = false;
        Detail::tick_key_up[i] = false;
    }

    Detail::mouse_delta_x = 0;
    Detail::mouse_delta_y = 0;
    Detail::mouse_pos_x = 0;
    Detail::mouse_pos_y = 0;
}

bool get_key_state(KeyCode key) { return Detail::keys_state[key]; }

bool is_key_down(KeyCode key) { return Detail::tick_key_down[key]; }

bool is_key_up(KeyCode key) { return Detail::tick_key_up[key]; }

std::tuple<int32_t, int32_t> get_mouse_move() { return std::make_tuple(Detail::mouse_delta_x, Detail::mouse_delta_y); }

std::tuple<int32_t, int32_t> get_mouse_pos() {
    return std::make_tuple(Detail::mouse_pos_x, Detail::mouse_pos_y);
}
} // namespace Input
} // namespace Goonya