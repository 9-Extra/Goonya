#pragma once

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
    template <typename F> requires std::invocable<F>
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
};

extern ThreadPool THREAD_POOL; // 全局的线程池

} // namespace Goonya