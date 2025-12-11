#include "ThreadPool.h"

#include "ThreadUtils.h"
#include <format>
#include <functional>
#include <thread>

namespace Goonya{

ThreadPool::ThreadPool() : stop(false) {
    // 为主线程和渲染线程（暂无）留出空间，并且至少有一个工作线程
    uint32_t threads = std::thread::hardware_concurrency() > 2 ? std::thread::hardware_concurrency() - 2 : 1;

    for (uint32_t i = 0; i < threads; ++i) {
        workers.emplace_back([this, i] {
            set_current_thread_name(std::format("Worker {}", i));
            for (;;) {
                std::move_only_function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                    if (this->stop && this->tasks.empty()) {
                        return;
                    }

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
        
                task(); // packaged_task抛出的异常会在future.get()时抛出
            }
        });
    }
}
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers) {
        worker.join();
    }
}

ThreadPool THREAD_POOL;

} // namespace Goonya