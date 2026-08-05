#include <gtest/gtest.h>

#include <atomic>
#include <exception>
#include <semaphore>
#include <thread>

#include "core/Future.h"
#include "core/ThreadPool.h"
#include "core/ThreadUtils.h"

using namespace Goonya;

// 与 test_future 相同的泵等待工具：测试线程扮演渲染线程泵队列，另一个线程阻塞取值
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

// Future<void> 版本：只等完成，不取值
static void pump_wait(Ref<Future<void>> future) {
    std::exception_ptr err;
    std::atomic<bool> done{false};

    std::thread taker([&] {
        try {
            future->take_result();
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
}

// 引擎约定 THREAD_POOL 必须在静态析构前手动关闭，且 stop_all 要求当前线程是渲染线程
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    current_thread_type = ThreadType::RENDER;
    int result = RUN_ALL_TESTS();
    THREAD_POOL.stop_all();
    return result;
}

// ---------------- 基本功能 ----------------

TEST(FutureVoidBasic, AsyncCompletes) {
    std::atomic<bool> ran{false};
    auto future = Future<void>::async(ThreadType::WORKER, [&] { ran = true; });
    pump_wait(future);
    EXPECT_TRUE(ran.load());
}

TEST(FutureVoidBasic, AsyncWithStopToken) {
    std::atomic<bool> got_valid_token{false};
    auto future =
        Future<void>::async(ThreadType::WORKER, [&](std::stop_token st) { got_valid_token = st.stop_possible(); });
    pump_wait(future);
    EXPECT_TRUE(got_valid_token.load());
}

// void -> 值：then 的返回类型推导为 Future<int>
TEST(FutureVoidBasic, ThenProducesValue) {
    auto future = Future<void>::async(ThreadType::WORKER, [] {})->then(ThreadType::WORKER, [] { return 42; });
    EXPECT_EQ(pump_take(future), 42);
}

// 值 -> void：then 返回 Future<void>
TEST(FutureVoidBasic, ThenReturnsVoid) {
    std::atomic<int> captured{0};
    auto future = Future<int>::async(ThreadType::WORKER, [] { return 42; })->then(ThreadType::WORKER, [&](int x) {
        captured = x;
    });
    pump_wait(future);
    EXPECT_EQ(captured.load(), 42);
}

TEST(FutureVoidBasic, ThenOnCompletedVoid) {
    std::atomic<bool> ran{false};
    auto future = Future<void>::completed()->then(ThreadType::WORKER, [&] { ran = true; });
    pump_wait(future);
    EXPECT_TRUE(ran.load());
}

// ---------------- 异常 ----------------

// void 任务抛异常，应穿透到后继
TEST(FutureVoidException, PropagatesFromVoidTask) {
    std::atomic<bool> continuation_ran{false};

    auto future = Future<void>::async(ThreadType::WORKER, [] {
                      throw std::runtime_error{"boom"};
                  })->then(ThreadType::WORKER, [&] {
        continuation_ran = true;
        return 1;
    });

    EXPECT_THROW(pump_take(future), std::runtime_error);
    EXPECT_FALSE(continuation_ran.load());
}

// 异常穿透到返回 void 的后继
TEST(FutureVoidException, PropagatesToVoidContinuation) {
    std::atomic<bool> continuation_ran{false};

    auto future = Future<int>::async(ThreadType::WORKER, []() -> int {
                      throw std::runtime_error{"boom"};
                  })->then(ThreadType::WORKER, [&](int) { continuation_ran = true; });

    EXPECT_THROW(pump_wait(future), std::runtime_error);
    EXPECT_FALSE(continuation_ran.load());
}

// ---------------- 取消 ----------------

// 检查点 1：排队期取消，void 任务体不执行
TEST(FutureVoidCancel, BeforeStart) {
    std::atomic<bool> task_ran{false};
    std::stop_source canceller;
    canceller.request_stop();

    auto future = Future<void>::async(ThreadType::WORKER, canceller, [&] { task_ran = true; });

    EXPECT_THROW(pump_wait(future), TaskCancel);
    EXPECT_FALSE(task_ran.load());
}

// 检查点 2：void 任务运行中协作取消，从链尾发起
TEST(FutureVoidCancel, MidExecution) {
    std::binary_semaphore started{0}, gate{0};
    std::atomic<bool> continuation_ran{false};

    auto future = Future<void>::async(ThreadType::WORKER, [&](std::stop_token st) {
        started.release();
        gate.acquire();
        if (st.stop_requested()) {
            throw TaskCancel{};
        }
    });
    auto tail = future->then(ThreadType::WORKER, [&] {
        continuation_ran = true;
        return 1;
    });

    started.acquire();
    tail->cancel();
    gate.release();

    EXPECT_THROW(pump_take(tail), TaskCancel);
    EXPECT_FALSE(continuation_ran.load());
}

// 检查点 3：头部算完，但提交 void 后继前发现已取消
TEST(FutureVoidCancel, BeforeSubmit) {
    std::binary_semaphore started{0}, gate{0};
    std::atomic<bool> head_finished{false};
    std::atomic<bool> continuation_ran{false};

    auto head = Future<int>::async(ThreadType::WORKER, [&] {
        started.release();
        gate.acquire();
        head_finished = true;
        return 1;
    });
    auto tail = head->then(ThreadType::WORKER, [&](int) { continuation_ran = true; });

    started.acquire();
    tail->cancel();
    gate.release();

    EXPECT_THROW(pump_wait(tail), TaskCancel);
    EXPECT_TRUE(head_finished.load());
    EXPECT_FALSE(continuation_ran.load());
}
