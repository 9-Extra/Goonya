#include "GameClock.h"
#include "runtime/GAssert.h"

namespace Goonya {
GameClock GAME_CLOCK;
void GameClock::start() noexcept {
    fixed_current_tick = 0;
    expected_tick = 0;
    virtual_total = ClockType::duration::zero();
    last_fixed_tick_time = ClockType::duration::zero();
    last_frame_time = ClockType::now();
}

void GameClock::update() noexcept {
    // ------------------处理frame_delta-------------------
    ClockType::time_point now = ClockType::now();
    ClockType::duration frame_delta = now - last_frame_time;
    last_frame_time = now;

    if (frame_delta > MAX_FRAME_INTERVAL) {
#if 0
            LOG_WARN("Lagged: Last frame takes {}ms",
                      std::chrono::duration_cast<std::chrono::milliseconds>(frame_delta).count());
#endif
        frame_delta = MAX_FRAME_INTERVAL;
    }

    virtual_delta = frame_delta;
    virtual_total += virtual_delta; // 没有精度问题

    // ------------------处理fixed_tick-------------------
    GN_ASSERT(fixed_current_tick == expected_tick);
    uint32_t tick = (virtual_total - last_fixed_tick_time) / FIXED_TICK_INTERVAL;
    last_fixed_tick_time = last_fixed_tick_time + tick * FIXED_TICK_INTERVAL;

    expected_tick = fixed_current_tick + tick;
}

bool GameClock::advance_fixed_tick() noexcept {
    if (fixed_current_tick < expected_tick) {
        fixed_current_tick++;
        return true;
    }
    return false;
}
} // namespace Goonya