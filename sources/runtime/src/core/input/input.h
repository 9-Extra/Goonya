#pragma once

#include <cstdint>
#include <tuple>

namespace Goonya {
namespace Input {

using KeyCode = uint32_t;
const size_t MAX_KEYCODE = 256;

void reset_state();
bool get_key_state(KeyCode key);

bool is_key_down(KeyCode key);
bool is_key_up(KeyCode key);

std::tuple<int32_t, int32_t> get_mouse_move();
std::tuple<int32_t, int32_t> get_mouse_pos();

} // namespace Input
} // namespace Goonya