#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <optional>
#include <semaphore>
#include <thread>

#include "core/Future.h"
#include "core/ThreadPool.h"
#include "core/ThreadUtils.h"

using namespace Goonya;

// 测试线程扮演渲染线程：在另一个线程阻塞等待结果的同时，泵 LOGIC/RENDER 队列，
// 否则终点在 RENDER 线程的任务链会死锁（参见 Future 设计中"等待线程必须泵自己的队列"）
template <typename T>
static T pump_take(Ref<Future<T>> future) {
    std::optional<T> out;
    std::exception_ptr err;
    std::atomic<bool> done{false};

    std::thread taker([&] {
        try {
            out.emplace(future->take_result());
        } catch (...) {
            err = std::current_exception();
        }
        done.store(true, std::memory_order::release);
    });

    while (!done.load(std::memory_order::acquire)) {
        main_thread_process();
        renderer_thread_process();
        std::this_thread::yield();
    }
    taker.join();

    if (err) {
        std::rethrow_exception(err);
    }
    return std::move(*out);
}

// 引擎约定 THREAD_POOL 必须在静态析构前手动关闭（其析构函数有断言），
// 且 stop_all 要求当前线程是渲染线程；测试线程扮演渲染线程负责泵队列
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    current_thread_type = ThreadType::RENDER;
    int result = RUN_ALL_TESTS();
    THREAD_POOL.stop_all();
    return result;
}

// ---------------- 基本功能 ----------------

TEST(FutureBasic, AsyncReturnsValue) {
    auto future = Future<int>::async(ThreadType::WORKER, [] { return 42; });
    EXPECT_EQ(pump_take(future), 42);
}

TEST(FutureBasic, ChainAcrossThreads) {
    std::atomic<bool> worker_continuation_on_worker{false};
    std::atomic<bool> render_continuation_on_render{false};

    auto future = Future<int>::async(ThreadType::WORKER, [] { return 5; })
                      ->then(ThreadType::WORKER,
                             [&](int x) {
                                 worker_continuation_on_worker = true;
                                 return x * 100;
                             })
                      ->then(ThreadType::RENDER, [&](int x) {
                          render_continuation_on_render = (current_thread_type == ThreadType::RENDER);
                          return x + 1;
                      });

    EXPECT_EQ(pump_take(future), 501);
    EXPECT_TRUE(worker_continuation_on_worker.load());
    EXPECT_TRUE(render_continuation_on_render.load());
}

TEST(FutureBasic, ThenOnCompletedFuture) {
    auto future = Future<int>::completed(5)->then(ThreadType::WORKER, [](int x) { return x + 1; });
    EXPECT_EQ(pump_take(future), 6);
}

TEST(FutureBasic, MoveOnlyResult) {
    auto future = Future<std::unique_ptr<int>>::async(ThreadType::WORKER, [] {
                      return std::make_unique<int>(7);
                  })->then(ThreadType::WORKER, [](std::unique_ptr<int> p) { return std::make_unique<int>(*p + 1); });

    std::unique_ptr<int> result = pump_take(future);
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(*result, 8);
}

// ---------------- 异常 ----------------

TEST(FutureException, PropagatesThroughChain) {
    std::atomic<bool> continuation_ran{false};

    auto future = Future<int>::async(ThreadType::WORKER, []() -> int {
                      throw std::runtime_error{"boom"};
                  })->then(ThreadType::WORKER, [&](int x) {
        continuation_ran = true;
        return x;
    });

    EXPECT_THROW(pump_take(future), std::runtime_error);
    EXPECT_FALSE(continuation_ran.load()); // 异常跳过后继
}

// ---------------- 取消 ----------------

// 检查点 1：任务还没开始跑就被取消，函数体不应执行
TEST(FutureCancel, BeforeStart) {
    std::atomic<bool> task_ran{false};
    std::stop_source canceller;
    canceller.request_stop(); // 预先取消，任务起跑时必然看到

    auto future = Future<int>::async(ThreadType::WORKER, canceller, [&] {
        task_ran = true;
        return 1;
    });

    EXPECT_THROW(pump_take(future), TaskCancel);
    EXPECT_FALSE(task_ran.load());
}

// 检查点 2：运行中的任务通过 stop_token 协作式中断，后继不执行
TEST(FutureCancel, MidExecution) {
    std::binary_semaphore started{0}, gate{0};
    std::atomic<bool> continuation_ran{false};

    auto future = Future<int>::async(ThreadType::WORKER, [&](std::stop_token st) {
        started.release(); // 握手：确认已越过检查点 1
        gate.acquire();    // 阻塞，等待主线程发出取消
        if (st.stop_requested()) {
            throw TaskCancel{};
        }
        return 1;
    });
    auto tail = future->then(ThreadType::WORKER, [&](int x) {
        continuation_ran = true;
        return x;
    });

    started.acquire(); // 确认任务已在运行，否则取消可能落在检查点 1 上
    tail->cancel();    // 从链尾取消整条链
    gate.release();

    EXPECT_THROW(pump_take(tail), TaskCancel);
    EXPECT_FALSE(continuation_ran.load());
}

// 检查点 3：任务正常算完，但提交后继前发现已取消，后继不应执行
TEST(FutureCancel, BeforeSubmit) {
    std::binary_semaphore started{0}, gate{0};
    std::atomic<bool> head_finished{false};
    std::atomic<bool> continuation_ran{false};

    auto head = Future<int>::async(ThreadType::WORKER, [&] {
        started.release(); // 握手：确认已越过检查点 1
        gate.acquire();    // 等主线程完成取消，保证返回时链已取消
        head_finished = true;
        return 1;
    });
    auto tail = head->then(ThreadType::WORKER, [&](int x) {
        continuation_ran = true;
        return x;
    });

    started.acquire();
    tail->cancel();
    gate.release();

    EXPECT_THROW(pump_take(tail), TaskCancel);
    EXPECT_TRUE(head_finished.load());     // 头部确实算完了
    EXPECT_FALSE(continuation_ran.load()); // 但结果没有提交给后继
}

// 取消是 best-effort：不检查 token 的任务照常跑完并产出正常结果
TEST(FutureCancel, NonCooperativeTaskStillReturnsValue) {
    std::binary_semaphore started{0}, gate{0};

    auto future = Future<int>::async(ThreadType::WORKER, [&] {
        started.release(); // 握手：确认已越过检查点 1
        gate.acquire();    // 阻塞期间主线程取消
        return 42;         // 不检查 token，正常返回
    });

    started.acquire();
    future->cancel();
    gate.release();

    EXPECT_EQ(pump_take(future), 42);
}

// 文档行为：已完成的任务再取消已经晚了，结果仍可正常取出
TEST(FutureCancel, LateCancelKeepsResult) {
    auto future = Future<int>::completed(42);
    future->cancel();
    EXPECT_EQ(pump_take(future), 42);
}
