#pragma once

#include <deque>
#include <mutex>
#include <optional>

namespace Goonya {

// "有"锁队列
template<typename T>
class LockQueue final{
private:
    std::mutex mutex;
    std::deque<T> deque;
public:
    LockQueue() = default;
    LockQueue(LockQueue&) = delete;

    void push_front(T t) noexcept {
        std::lock_guard lock(mutex);
        deque.push_front(std::move(t));
    }

    std::optional<T> pop_front() noexcept {
        std::lock_guard lock(mutex);
        if (deque.empty()){
            return std::nullopt;
        } else {
            std::optional<T> front{std::move(deque.front())};
            deque.pop_front();
            return front;
        }
    }

    void push_back(T t) noexcept {
        std::lock_guard lock(mutex);
        deque.push_back(std::move(t));
    }

    std::optional<T> pop_back() noexcept {
        std::lock_guard lock(mutex);
        if (deque.empty()){
            return std::nullopt;
        } else {
            std::optional<T> front{std::move(deque.back())};
            deque.pop_back();
            return front;
        }
    }


};

}