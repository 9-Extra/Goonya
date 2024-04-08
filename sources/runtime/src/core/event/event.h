#pragma once

#include <vector>
#include <unordered_map>

#include <runtime/log/Log.h>

namespace Goonya {
namespace Event {

using ListenerID = uint32_t;

namespace Detail {

// template <typename T> class UIDAlloctor {
// public:
//     template <typename M> inline constexpr static uint32_t id() { return i++; }
//     inline static uint32_t i = 1;
// };

struct Listener{
    ListenerID id;
    int priority;
    void* args;
    bool (*trigger)(void* args, void* event);
};

class EventBus{
public:
    void init(){
        uid = 0;
    }

    template<typename E>
    void dispatch_event(E e){
        const std::vector<Listener>& q = listeners[typeid(E).hash_code()];
        for(const Listener& l: q){
            bool handled = l.trigger(l.args, &e);
            if (handled) break;
        }
    }

    template<typename E, typename T>
    ListenerID subscribe_event(int priority, T* args, bool (*trigger)(T* args, E& event)){
        std::vector<Listener>& l = listeners[typeid(E).hash_code()];
        
        auto iter = l.begin();
        for(;iter != l.end();iter++){
            if (priority <= iter->priority){
                break;
            }
        }

        ListenerID id = ++uid;
        l.insert(iter, Listener{id, priority, args, (bool (*)(void*, void*))trigger});
        
        LOG_DEBUG("注册事件: {}，id: {}", typeid(E).name(), id);
        return id;
    }

    template<typename E>
    bool remove_listener(ListenerID id){
        std::vector<Listener>& l = listeners[typeid(E).hash_code()];
        auto iter = l.begin();
        for(;iter != l.end();iter++){
            if (id == iter->id){
                l.erase(iter);
                LOG_DEBUG("移除监听: {}，id: {}", typeid(E).name(), id);
                return true;
            }
        }
        return false;
    }

    // void dispatch(){
    //     for(const auto& q : listeners){
    //         for(const Listener& l : q){

    //         }
    //     }
    // }
private:
    std::unordered_map<size_t, std::vector<Listener>> listeners;
    ListenerID uid;
};

extern EventBus event_bus;

}

inline void initalize(){
    Detail::event_bus.init();
}

template<typename E>
void dispatch_event(E e){
    Detail::event_bus.dispatch_event(e);
}

template<typename E, typename T>
ListenerID subscribe_event(int priority, T* args, bool (*trigger)(T* args, E& event)){
    return Detail::event_bus.subscribe_event(priority, args, trigger);
}

template<typename E>
bool remove_listener(ListenerID id){
    return Detail::event_bus.remove_listener<E>(id);
}

} // namespace Event
} // namespace Goonya