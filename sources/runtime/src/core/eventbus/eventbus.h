#pragma once

#include <unordered_map>
#include <vector>

#include "core/log/Log.h"

namespace Goonya {
namespace EventBus {

using ListenerID = uint32_t;

namespace Detail {

// template <typename T> class UIDAlloctor {
// public:
//     template <typename M> inline constexpr static uint32_t id() { return i++;
//     } inline static uint32_t i = 1;
// };

struct Listener {
    ListenerID id;
    int priority;
    void *args;
    bool (*trigger)(void *args, void *event) noexcept(false);
};

class EventBus {
public:
    void init() noexcept { uid = 0; }

    template <bool CATCH_EXCEPTION = false, typename E>
    void dispatch_event(E e) noexcept(CATCH_EXCEPTION) {
        const std::vector<Listener> &q = listeners[typeid(E).hash_code()];
        for (const Listener &l : q) {
            if constexpr (CATCH_EXCEPTION) {
                try {
                    bool handled = l.trigger(l.args, &e);
                    if (handled)
                        break;
                } catch (const std::exception &e) {
                    LOG_ERROR("在处理事件 {} 时发生异常：{}", typeid(E).name(), e.what());
                }
            } else {
                bool handled = l.trigger(l.args, &e);
                if (handled)
                    break;
            }
        }
    }

    template <typename E, typename T>
    ListenerID subscribe_event(int priority, T *args, bool (*trigger)(T *args, E &event)) noexcept {
        std::vector<Listener> &l = listeners[typeid(E).hash_code()];

        auto iter = l.begin();
        for (; iter != l.end(); iter++) {
            if (priority <= iter->priority) {
                break;
            }
        }

        ListenerID id = ++uid;
        l.insert(iter, Listener{id, priority, args, (bool (*)(void *, void *))trigger});
        LOG_DEBUG("注册事件: {}，id: {}", typeid(E).name(), id);
        return id;
    }

    template <typename E>
    bool remove_listener(ListenerID id) noexcept {
        std::vector<Listener> &l = listeners[typeid(E).hash_code()];
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
    std::unordered_map<size_t, std::vector<Listener>> listeners;
    ListenerID uid;
};

extern EventBus event_bus;

} // namespace Detail

inline void initalize() { Detail::event_bus.init(); }

template <bool CATCH_EXCEPTION = false, typename E>
void dispatch_event(E e) {
    Detail::event_bus.dispatch_event<CATCH_EXCEPTION, E>(e);
}

template <typename E, typename T>
ListenerID subscribe_event(int priority, T *args, bool (*trigger)(T *args, E &event)) {
    return Detail::event_bus.subscribe_event(priority, args, trigger);
}

template <typename E>
bool remove_listener(ListenerID id) {
    return Detail::event_bus.remove_listener<E>(id);
}

} // namespace EventBus
} // namespace Goonya