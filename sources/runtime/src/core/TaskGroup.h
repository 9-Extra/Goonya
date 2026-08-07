#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "core/Task.h"

namespace Goonya {

class TaskGroup {
private:
    std::atomic<uint32_t> running_tasks{0};
    std::mutex mutex;
    std::condition_variable blocker;

public:
    TaskGroup() = default;
    TaskGroup(const TaskGroup &) = delete;
    TaskGroup(TaskGroup &&) = delete;

    ~TaskGroup() { join(); }

    void spawn(Task<void> &&task) { wrap(std::move(task)).launch(); }

    void join() {
        std::unique_lock lock{mutex};
        blocker.wait(lock, [&t = running_tasks]() { return t.load(std::memory_order::acquire) == 0; });
    }

private:
    void on_task_start() noexcept { running_tasks.fetch_add(1, std::memory_order::relaxed); }
    void on_task_complete() noexcept {
        if (running_tasks.fetch_sub(1, std::memory_order::acq_rel) == 1) {
            {
                // 空锁块:强制等到 join 里的 waiter 真正入睡后再 notify,防止丢唤醒
                std::lock_guard lock{mutex};
            }
            blocker.notify_all();
        }
    }

    // 成员协程:this 被保存在帧里,TaskGroup 必须活得比所有 spawn 的任务久(由析构时的 join 保证)
    Task<void> wrap(Task<void> task) {
        struct Guard {
            TaskGroup &g;
            explicit Guard(TaskGroup &g) : g(g) { g.on_task_start(); }
            Guard(const Guard &) = delete;
            Guard(Guard &&) = delete;
            ~Guard() { g.on_task_complete(); }
        } guard{*this};

        co_await std::move(task);
    }
};

} // namespace Goonya