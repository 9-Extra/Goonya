#include "input.h"
#include "core/events.h"
#include "platform/display/display.h"
#include <core/eventbus/eventbus.h>
#include <cstddef>
#include <cstdint>

namespace Goonya::Input {

namespace Detail {

KeyState keys_state_last_tick[MAX_KEYCODE];
KeyState keys_state[MAX_KEYCODE];
int32_t mouse_delta_x;
int32_t mouse_delta_y;
int32_t mouse_pos_x;
int32_t mouse_pos_y;

KeyState mouse_key_state_last_tick[MouseKey::MOUSE_KEY_MAX];
KeyState mouse_key_state[MouseKey::MOUSE_KEY_MAX];

} // namespace Detail

void tick_update() {
    using namespace Detail;
    for (size_t i = 0; i < MAX_KEYCODE; i++) {
        keys_state_last_tick[i] = keys_state[i];
    }
    for (size_t i = 0; i < MOUSE_KEY_MAX; i++) {
        mouse_key_state_last_tick[i] = mouse_key_state[i];
    }
    mouse_delta_x = 0;
    mouse_delta_y = 0;
}

void initalize() {
    using namespace Detail;
    reset_state();
    EventBus::subscribe_event<Events::SysKeyEvent>(0, [](Events::SysKeyEvent &e) {
        keys_state[(uint32_t)e.key] = e.state;
        return false;
    });
    EventBus::subscribe_event<Events::SysMousePos>(0, [](Events::SysMousePos &e) {
        mouse_delta_x = (int32_t)e.x - mouse_pos_x;
        mouse_delta_y = (int32_t)e.y - mouse_pos_y;
        mouse_pos_x = (int32_t)e.x;
        mouse_pos_y = (int32_t)e.y;
        return false;
    });
    EventBus::subscribe_event<Events::SysMouseClick>(0, [](Events::SysMouseClick &e) {
        mouse_key_state[(uint32_t)e.key] = e.state;
        return false;
    });
    EventBus::subscribe_event<Events::SysWindowDeActive>(0, [](Events::SysWindowDeActive &e) {
        tick_update(); // 失去焦点时抬起所有按键
        // LOG_TRACE("窗口失去焦点");
        return false;
    });

    EventBus::subscribe_event<Events::PostTick>(100, [](Events::PostTick &e) {
        tick_update();
        return false;
    });
}

void reset_state() {
    for (size_t i = 0; i < MAX_KEYCODE; i++) {
        Detail::keys_state[i] = KeyState::UP;
        Detail::keys_state_last_tick[i] = KeyState::UP;
    }

    for (size_t i = 0; i < MouseKey::MOUSE_KEY_MAX; i++) {
        Detail::mouse_key_state[i] = KeyState::UP;
        Detail::mouse_key_state_last_tick[i] = KeyState::UP;
    }

    Detail::mouse_delta_x = 0;
    Detail::mouse_delta_y = 0;
    Detail::mouse_pos_x = 0;
    Detail::mouse_pos_y = 0;
}

KeyState get_key_state(KeyCode key) { return Detail::keys_state[(uint32_t)key]; }

bool is_key_click(KeyCode key) {
    return Detail::keys_state_last_tick[(uint32_t)key] == KeyState::UP &&
           Detail::keys_state[(uint32_t)key] == KeyState::DOWN;
}

bool is_key_release(KeyCode key) {
    return Detail::keys_state_last_tick[(uint32_t)key] == KeyState::DOWN &&
           Detail::keys_state[(uint32_t)key] == KeyState::UP;
}

std::tuple<int32_t, int32_t> get_mouse_move() { return std::make_tuple(Detail::mouse_delta_x, Detail::mouse_delta_y); }

std::tuple<int32_t, int32_t> get_mouse_pos() { return std::make_tuple(Detail::mouse_pos_x, Detail::mouse_pos_y); }

KeyState get_mouse_state(MouseKey key) { return Detail::mouse_key_state[key]; }
bool is_mouse_click(MouseKey key) {
    return Detail::mouse_key_state[key] == KeyState::UP && Detail::mouse_key_state_last_tick[key] == KeyState::DOWN;
}
bool is_mouse_release(MouseKey key) {
    return Detail::mouse_key_state[key] == KeyState::DOWN && Detail::mouse_key_state_last_tick[key] == KeyState::UP;
}

} // namespace Goonya::Input
