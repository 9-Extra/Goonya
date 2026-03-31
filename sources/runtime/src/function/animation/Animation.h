#pragma once

#include "core/cgmath/quaternion.h"
#include "core/cgmath/vector.h"
#include "core/clock/GameClock.h"
#include "function/world/GObject.h"
#include "resource/Resource.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Goonya {

enum class InterpolationType {
    STEP,
    LINEAR,
};

template <typename T>
struct KeyPoint {
    float time;
    T value;
};

template <typename T>
struct TimeSeries {
    std::vector<KeyPoint<T>> key_points;

    float duration() const { return key_points.empty() ? 0 : key_points.back().time; }

    T interpolate(float time, InterpolationType interpolation_type) const {
        if (key_points.empty()) {
            return T{};
        }
        if (key_points.size() == 1) {
            return key_points[0].value;
        }
        // 找到第一个大于等于time的键点索引
        auto it = std::ranges::lower_bound(key_points, time, std::ranges::less{},
                                           [](const KeyPoint<T> &a) { return a.time; });

        if (it == key_points.end()) {
            // 如果time大于所有键点的时间，返回最后一个键点的值
            return key_points.back().value;
        }

        if (it == key_points.begin()) {
            // 如果time小于等于第一个键点的时间，返回第一个键点的值
            return it->value;
        }

        switch (interpolation_type) {
        case InterpolationType::STEP: {
            return it->value;
        }
        case InterpolationType::LINEAR: {
            const KeyPoint<T> &prev = *(it - 1);
            const KeyPoint<T> &next = *it;
            float t = (time - prev.time) / (next.time - prev.time);

            if constexpr (std::is_same_v<T, Quaternion>) {
                // 球面线性插值
                return Quaternion::slerp(prev.value, next.value, t);
            } else {
                // 线性插值
                return prev.value * (1.0f - t) + next.value * t;
            }
        }
        default: {
            std::unreachable();
        }
        }
    }
};

class Animation : public Resource {
public:
    struct Channel {
        std::string target;
        InterpolationType interpolation_type;

        GObject *get_target_object(GObject *root) const noexcept {
            if (!root) return nullptr;
            return root->get_child_by_path(target).get();
        }

        virtual ~Channel() = default;

        virtual void apply(GObject *root, float time_offset) = 0;
        virtual bool is_vaild_on(GObject *root) noexcept { return get_target_object(root) != nullptr; }
    };

    struct PositionChannel : public Channel {
        TimeSeries<Vector3f> position_series;

        void apply(GObject *root, float time_offset) override {
            GObject *t = get_target_object(root);
            if (!t) return;
            t->set_local_position(position_series.interpolate(time_offset, interpolation_type));
        }
    };

    struct RotationChannel : public Channel {
        TimeSeries<Quaternion> rotation_series;
        void apply(GObject *root, float time_offset) override {
            GObject *t = get_target_object(root);
            if (!t) return;
            t->set_local_rotation(rotation_series.interpolate(time_offset, interpolation_type));
        }
    };

    struct ScaleChannel : public Channel {
        TimeSeries<Vector3f> scale_series;
        void apply(GObject *root, float time_offset) override {
            GObject *t = get_target_object(root);
            if (!t) return;
            t->set_local_scale(scale_series.interpolate(time_offset, interpolation_type));
        }
    };

    std::vector<std::unique_ptr<Channel>> channels;
    GameClock::Duration duration = GameClock::Duration::zero(); // 动画持续的时长
public:
    Animation() = default;
};

} // namespace Goonya