#pragma once

#include "core/clock/GameClock.h"
#include "core/enum_operator.h"
#include "core/log/Log.h"
#include "function/animation/Animation.h"
#include "function/world/Component.h"
#include "function/world/World.h"
#include <chrono>
#include <utility>
#include <vector>

namespace Goonya {

enum class AnimationPlayMode {
    DISABLE = 0,
    ONCE = 1,     // >
    LOOP = 2,     // >>>>>...
    PINGPONG = 3, // ><><><>...

    PLAY_MASK = 3,

    REVERSE_FLAG = 4
};

DECLARE_ENUM_OPERATORS(AnimationPlayMode);

class CpntAnimator : public Component, public TickFunction {
private:
    Ref<Animation> animation;
    AnimationPlayMode mode = AnimationPlayMode::LOOP;
    GameClock::TimePoint start_time = GameClock::TimePoint{}; // 动画开始播放的时间
    // 轨道→目标节点的绑定缓存，在注册/设置动画时解析一次；不随 clone 复制，注册时重新解析
    std::vector<std::pair<Animation::Channel *, GObject *>> bindings;

    void resolve_bindings() noexcept {
        bindings.clear();
        if (!animation) return;

        GObject *owner = get_owner();
        for (const auto &c : animation->channels) {
            GObject *target = c->get_target_object(owner);
            if (!target) {
                LOG_WARN("动画目标\"{}\"不匹配", c->target);
                continue;
            }
            bindings.emplace_back(c.get(), target);
        }
    }

public:
    CpntAnimator() : TickFunction(TickType::TICK) {}

    void set_animation(const Ref<Animation> &ani) noexcept {
        animation = ani;

        if (!is_registered()) return;
        resolve_bindings();
    }
    Ref<Animation> get_animation() const noexcept { return animation; }

    std::unique_ptr<Component> clone() const override {
        auto new_comp = std::make_unique<CpntAnimator>();
        new_comp->animation = animation;
        new_comp->mode = mode;
        new_comp->start_time = start_time;
        return new_comp;
    }

protected:
    void on_register() override {
        register_ticker(get_owner()->get_world());
        resolve_bindings();
    }

    void on_unregister() override {
        unregister_ticker();
        bindings.clear();
    }

    void tick() override {
        GameClock::TimePoint now = GAME_CLOCK.now();
        if (!animation || mode == AnimationPlayMode::DISABLE || animation->duration == GameClock::Duration::zero() ||
            now < start_time) {
            return;
        }
        GameClock::Duration delta = now - start_time;

        bool reverse = contain(mode, AnimationPlayMode::REVERSE_FLAG);
        switch (mode & AnimationPlayMode::PLAY_MASK) {
        case AnimationPlayMode::ONCE: {
            if (delta > animation->duration) {
                return;
            }
            break;
        }
        case AnimationPlayMode::LOOP: {
            delta %= animation->duration;
            break;
        }
        case AnimationPlayMode::PINGPONG: {
            delta %= animation->duration;
            reverse ^= (delta / animation->duration) % 2 != 0;
            break;
        }

        default: {
            std::unreachable();
        }
        }

        if (reverse) {
            delta = animation->duration - delta;
        }

        // 转化为秒计数
        float time_offset = std::chrono::duration_cast<std::chrono::duration<float>>(delta).count();

        for (const auto &[channel, target] : bindings) {
            channel->apply_to(target, time_offset);
        }
    }
};
} // namespace Goonya