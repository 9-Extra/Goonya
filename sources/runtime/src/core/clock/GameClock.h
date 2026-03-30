#pragma once

#include <chrono>
#include <cstdint>

namespace Goonya {

/**
 * @brief 游戏内时间
 * 与真实时间不同，游戏内时间是模拟时间，因为可能出现卡顿，可能比现实世界慢一点，以固定时间间隔tick_count增加。
 * 时间分为两种，游戏刻时间和渲染时间。
 * 游戏刻时间是模拟时间，以固定时间间隔tick_count增加。
 * 渲染时间是对游戏刻时间的插值，这样当游戏刻卡顿时，渲染也应当相应变慢
 */
class GameClock {
public:
    using ClockType = std::chrono::steady_clock;
    using TimePoint = ClockType::time_point;
    using Duration = ClockType::duration;

    static const uint32_t TPS = 20;
    static const uint32_t MIN_FPS = 20;
    static constexpr ClockType::duration FIXED_TICK_INTERVAL =
        std::chrono::duration_cast<ClockType::duration>(std::chrono::seconds(1)) / TPS; // 50 ms
    static constexpr ClockType::duration MAX_FRAME_INTERVAL =
        std::chrono::duration_cast<ClockType::duration>(std::chrono::seconds(1)) / MIN_FPS; // 50 ms
private:
    uint64_t fixed_current_tick = 0;
    uint64_t expected_tick = 0;
    Duration last_fixed_tick_time = ClockType::duration::zero();

    Duration virtual_delta = ClockType::duration::zero();
    Duration virtual_total = ClockType::duration::zero();
    TimePoint last_frame_time;

public:
    GameClock() = default;

    uint64_t current_tick() const noexcept { return fixed_current_tick; }

    TimePoint fixed_now() const noexcept { return ClockType::time_point{} + fixed_total(); }

    ClockType::duration fixed_total() const noexcept { return fixed_current_tick * FIXED_TICK_INTERVAL; }

    /**
     * @brief 获取当前帧对应时间点
     * @note 本非真实时间，而是游戏世界模拟时间
     * @return TimePoint 当前时间点
     */
    TimePoint now() const noexcept { return ClockType::time_point{} + virtual_total; }

    Duration total() const noexcept { return virtual_total; }

    /**
     * @brief 获取当前帧的模拟时间间隔
     * @note 对于游戏世界模拟时间来说，这是个定值
     * @return Duration 当前模拟时间间隔
     */
    constexpr Duration delta() const noexcept // NOLINT，与其他类似函数保持一致
    {
        return virtual_delta;
    }

private:
    friend void main_loop();

    /**
     * @brief 作为整个游戏的时间起点
     */
    void start() noexcept;
    void update() noexcept;
    bool advance_fixed_tick() noexcept;
};

extern GameClock GAME_CLOCK;
} // namespace Goonya