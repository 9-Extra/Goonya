#pragma once

#include "core/LockQueue.h"
#include "core/ThreadUtils.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <concepts>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace Goonya {

// class Task: public RefCount{
// private:
//     std::function<void()> content;
//     std::atomic<uint32_t> prerequistites_count;
// public:

// private:

// };

class ThreadPool final {
private:
    std::vector<std::thread> workers;
    std::queue<std::move_only_function<void()>> tasks;

    LockQueue<std::move_only_function<void()>> main_thread_tasks;
    LockQueue<std::move_only_function<void()>> renderer_thread_tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool();
    ~ThreadPool();
    // 禁止拷贝和移动
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // 提交任务，返回future
    template <typename F>
        requires std::invocable<F>
    auto enqueue(F &&f) -> std::future<std::invoke_result_t<F>> {
        using return_type = std::invoke_result_t<F>;

        std::packaged_task<return_type()> task{std::forward<F>(f)};
        std::future<return_type> res = task.get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) {
                throw RuntimeError("enqueue on stopped ThreadPool");
            }

            tasks.emplace(std::move(task));
        }
        condition.notify_one();
        return res;
    }

    // 提交任务，但不返回
    template <typename F>
        requires std::invocable<F>
    void enqueue_detached(F &&f) {
        static_assert(std::is_same_v<std::invoke_result_t<F>, void>);
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw RuntimeError("enqueue on stopped ThreadPool");
            }
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    template <typename F>
        requires std::invocable<F>
    void enqueue_main_thread(F &&f) noexcept {
        static_assert(std::is_same_v<std::invoke_result_t<F>, void>);
        main_thread_tasks.push(std::forward<F>(f));
    }

    template <typename F>
        requires std::invocable<F>
    void enqueue_renderer_thread(F &&f) noexcept {
        static_assert(std::is_same_v<std::invoke_result_t<F>, void>);
        renderer_thread_tasks.push(std::forward<F>(f));
    }

    void stop_all() noexcept;

private:
    friend void main_thread_process();
    friend void renderer_thread_process();
};

extern ThreadPool THREAD_POOL; // 全局的线程池

inline void main_thread_process() {
    auto &queue = THREAD_POOL.main_thread_tasks;
    for (auto task = queue.pop(); task.has_value(); task = queue.pop()) {
        task.value()();
    }
}

inline void renderer_thread_process() {
    GN_ASSERT(current_thread_type == ThreadType::RENDER);
    auto &queue = THREAD_POOL.renderer_thread_tasks;
    for (auto task = queue.pop(); task.has_value(); task = queue.pop()) {
        task.value()();
    }
}

} // namespace Goonya