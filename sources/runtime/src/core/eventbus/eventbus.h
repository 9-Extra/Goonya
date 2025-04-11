#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/log/Log.h"

namespace Goonya {

class EventBus {
public:
    EventBus() = delete;

    using ListenerID = uint32_t;

    static void initalize() noexcept { uid = 0; }

    template <typename E>
    static void dispatch_event(E event) noexcept(false) {
        for (const Listener<E> &l : EventListeners<E>::listeners) {
            bool handled = l.trigger(event);
            if (handled)
                break;
        }
    }

    template <typename E>
    static void dispatch_event_no_exception(E event) noexcept {
        try {
            for (const Listener<E> &l : EventListeners<E>::listeners) {
                bool handled = l.trigger(event);
                if (handled)
                    break;
            }
        } catch (const std::exception &e) {
            LOG_ERROR("在处理事件 {} 时发生异常：{}", typeid(E).name(), e.what());
        }
    }

    template <typename E, typename T>
        requires std::is_convertible_v<T &&, std::function<bool(E &)>>
    static ListenerID subscribe_event(int priority, T &&trigger) noexcept {
        std::vector<Listener<E>> &l = EventListeners<E>::listeners;

        auto iter = l.begin();
        for (; iter != l.end(); iter++) {
            if (priority <= iter->priority) {
                break;
            }
        }

        ListenerID id = ++uid;
        l.insert(iter, Listener<E>{id, priority, std::forward<T>(trigger)});
        LOG_DEBUG("注册事件: {}，id: {}", typeid(E).name(), id);
        return id;
    }

    template <typename E>
    static bool remove_listener(ListenerID id) noexcept {
        std::vector<Listener<E>> &l = EventListeners<E>::listeners;
        auto iter = l.begin();
        for (; iter != l.end(); iter++) {
            if (id == iter->id) {
                l.erase(iter);
                LOG_DEBUG("移除监听: {}，id: {}", typeid(E).name(), id);
                return true;
            }
        }
        return false;
    }

private:
    template <typename E>
    struct Listener {
        ListenerID id;
        int priority;
        std::function<bool(E &)> trigger;
    };

    template <typename E>
    struct EventListeners {
        inline static std::vector<Listener<E>> listeners;
    };
    inline static ListenerID uid;
};

} // namespace Goonya