#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "core/Task.h"
#include "core/ThreadPool.h"
#include "core/ThreadUtils.h"

using namespace Goonya;
using namespace std::chrono_literals;

// 测试线程扮演渲染线程：泵 LOGIC/RENDER 队列直到 pred 成立。
// 终点在 RENDER 线程的任务必须靠泵来"等待"，直接阻塞会死锁
static bool pump_until(bool (*pred)(void *), void *ctx, std::chrono::milliseconds timeout = 5000ms) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!pred(ctx)) {
        main_thread_process();
        renderer_thread_process();
        std::this_thread::yield();
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
    }
    return true;
}

static bool pump_until_flag(std::atomic<bool> &flag) {
    return pump_until([](void *p) { return static_cast<std::atomic<bool> *>(p)->load(); }, &flag);
}

// 生命周期探针：活在协程帧里，构造/析构增减计数，用于验证帧的创建与销毁
struct LiveCounter {
    std::atomic<int> *count;
    explicit LiveCounter(std::atomic<int> *c) : count(c) { count->fetch_add(1); }
    LiveCounter(const LiveCounter &) = delete;
    LiveCounter &operator=(const LiveCounter &) = delete;
    LiveCounter(LiveCounter &&o) noexcept : count(std::exchange(o.count, nullptr)) {}
    ~LiveCounter() {
        if (count) {
            count->fetch_sub(1);
        }
    }
};

// 引擎约定 THREAD_POOL 必须在静态析构前手动关闭（其析构函数有断言），
// 且 stop_all 要求当前线程是渲染线程；测试线程扮演渲染线程负责泵队列
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    current_thread_type = ThreadType::RENDER;
    int result = RUN_ALL_TESTS();
    THREAD_POOL.stop_all();
    return result;
}

// ---------------- 测试用协程（协程必须是自由函数，不能是捕获式 lambda） ----------------

static Task<void> empty_body(std::atomic<bool> &ran, ThreadType &body_thread) {
    ran = true;
    body_thread = current_thread_type;
    co_return;
}

static Task<int> return_value(int v) { co_return v; }

static Task<void> hop_to_worker(ThreadType &stage1, ThreadType &stage2, std::atomic<bool> &done) {
    stage1 = current_thread_type;
    co_await switch_thread(ThreadType::WORKER);
    stage2 = current_thread_type;
    done = true;
    co_return;
}

static Task<void> hop_worker_then_render(ThreadType &s1, ThreadType &s2, ThreadType &s3, std::atomic<bool> &done) {
    s1 = current_thread_type;
    co_await switch_thread(ThreadType::WORKER);
    s2 = current_thread_type;
    co_await switch_thread(ThreadType::RENDER);
    s3 = current_thread_type;
    done = true;
    co_return;
}

static Task<void> same_thread_switch(std::atomic<bool> &after) {
    co_await switch_thread(ThreadType::RENDER); // 已在 RENDER,await_ready 短路
    after = true;
    co_return;
}

static Task<void> delayed_switch(std::atomic<bool> &after) {
    co_await switch_thread(ThreadType::RENDER, /*delay=*/true); // 强制推到帧尾
    after = true;
    co_return;
}

static Task<int> child_on_worker(int v) {
    co_await switch_thread(ThreadType::WORKER);
    co_return v;
}

static Task<void> parent_awaits_child(int &out, std::atomic<bool> &done) {
    out = co_await return_value(42);
    done = true;
    co_return;
}

static Task<void> void_child(bool &ran) {
    ran = true;
    co_return;
}

static Task<void> parent_awaits_void(bool &child_ran, std::atomic<bool> &done) {
    co_await void_child(child_ran);
    done = true;
    co_return;
}

static Task<int> level1() { co_return 1; }
static Task<int> level2() {
    int v = co_await level1();
    co_return v + 2;
}
static Task<int> level3() {
    int v = co_await level2();
    co_return v + 3;
}
static Task<void> nested_parent(int &out, std::atomic<bool> &done) {
    out = co_await level3();
    done = true;
    co_return;
}

static Task<int> throwing_child() {
    co_await switch_thread(ThreadType::WORKER);
    throw std::runtime_error{"boom"};
}

static Task<void> parent_catches(std::string &what, std::atomic<bool> &done) {
    try {
        co_await throwing_child();
    } catch (const std::runtime_error &e) {
        what = e.what();
    }
    done = true;
    co_return;
}

static Task<void> detached_throws(std::atomic<bool> &before_throw) {
    before_throw = true;
    // 注意:没有 co_ 关键字的函数不是协程,throw 会像普通函数一样同步抛给调用者
    co_await std::suspend_never{};
    throw std::runtime_error{"detached boom"};
}

// 组合语义锁定：parent 在 child 完成的线程上恢复（见 Task 文档："切换回来之后可能在任意线程上"）
static Task<void> parent_resumes_on_child_thread(ThreadType &after, std::atomic<bool> &done) {
    co_await child_on_worker(0);
    after = current_thread_type;
    done = true;
    co_return;
}

// ---------------- 基本功能 ----------------

TEST(TaskBasic, LazyDoesNotRunUntilLaunch) {
    std::atomic<bool> ran{false};
    ThreadType body_thread = ThreadType::UNKNOWN;
    {
        auto task = empty_body(ran, body_thread);
        EXPECT_FALSE(ran.load()); // lazy:创建不执行
        std::move(task).launch();
        EXPECT_TRUE(ran.load()); // launch 在当前线程同步执行完（体内无挂起点）
    }
}

TEST(TaskBasic, LaunchRunsOnCurrentThread) {
    std::atomic<bool> ran{false};
    ThreadType body_thread = ThreadType::UNKNOWN;
    auto task = empty_body(ran, body_thread);
    std::move(task).launch();
    EXPECT_EQ(body_thread, ThreadType::RENDER); // 测试线程扮演 RENDER
}

// ---------------- 线程切换 ----------------

TEST(TaskThread, SwitchToWorker) {
    ThreadType s1 = ThreadType::UNKNOWN, s2 = ThreadType::UNKNOWN;
    std::atomic<bool> done{false};
    hop_to_worker(s1, s2, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_EQ(s1, ThreadType::RENDER);
    EXPECT_EQ(s2, ThreadType::WORKER);
}

TEST(TaskThread, SwitchWorkerThenBackToRender) {
    ThreadType s1 = ThreadType::UNKNOWN, s2 = ThreadType::UNKNOWN, s3 = ThreadType::UNKNOWN;
    std::atomic<bool> done{false};
    hop_worker_then_render(s1, s2, s3, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_EQ(s1, ThreadType::RENDER);
    EXPECT_EQ(s2, ThreadType::WORKER);
    EXPECT_EQ(s3, ThreadType::RENDER);
}

TEST(TaskThread, SameThreadShortCircuits) {
    std::atomic<bool> after{false};
    same_thread_switch(after).launch();
    // await_ready 短路:不经过队列,launch 返回前已执行完
    EXPECT_TRUE(after.load());
}

TEST(TaskThread, DelayForcesQueueHop) {
    std::atomic<bool> after{false};
    delayed_switch(after).launch();
    EXPECT_FALSE(after.load()); // delay=true:被推入 RENDER 队列,launch 返回时还没跑
    renderer_thread_process();  // 泵一次
    EXPECT_TRUE(after.load());
}

// ---------------- 组合(co_await Task) ----------------

TEST(TaskCompose, AwaitChildReturnsValue) {
    int out = 0;
    std::atomic<bool> done{false};
    parent_awaits_child(out, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_EQ(out, 42);
}

TEST(TaskCompose, AwaitVoidChild) {
    bool child_ran = false;
    std::atomic<bool> done{false};
    parent_awaits_void(child_ran, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_TRUE(child_ran);
}

TEST(TaskCompose, NestedAwaitAccumulates) {
    int out = 0;
    std::atomic<bool> done{false};
    nested_parent(out, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_EQ(out, 6); // 1 + 2 + 3
}

TEST(TaskCompose, AwaitResumesOnChildThread) {
    // 行为锁定:co_await 之后 parent 在 child 完成的线程(WORKER)上恢复,
    // 需要亲和时必须显式 switch_thread 回来(当前设计决策:文档派)
    ThreadType after = ThreadType::UNKNOWN;
    std::atomic<bool> done{false};
    parent_resumes_on_child_thread(after, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_EQ(after, ThreadType::WORKER);
}

// ---------------- 异常 ----------------

TEST(TaskException, PropagatesThroughAwait) {
    std::string what;
    std::atomic<bool> done{false};
    parent_catches(what, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    EXPECT_EQ(what, "boom");
}

TEST(TaskException, DetachedExceptionIsContained) {
    // detached 任务的异常无人观察,由 FinalAwaiter 打日志后销毁帧,不能炸进程
    std::atomic<bool> before_throw{false};
    detached_throws(before_throw).launch();
    EXPECT_TRUE(before_throw.load());
    SUCCEED(); // 到达这里即未 terminate
}

// ---------------- 帧生命周期 ----------------

static Task<void> probe_in_param(LiveCounter probe) { co_return; }

static Task<void> probe_across_suspend(std::atomic<int> *count) {
    LiveCounter probe{count}; // 第一次 resume 才构造,跨挂起点活在帧里
    co_await switch_thread(ThreadType::WORKER);
    co_return;
}

static Task<void> parent_awaits_probe_child(std::atomic<int> *count, std::atomic<bool> &done) {
    co_await probe_across_suspend(count);
    done = true;
    co_return;
}

static Task<void> detached_probe(std::atomic<int> *count, std::atomic<bool> &done) {
    LiveCounter probe{count};
    co_await switch_thread(ThreadType::WORKER);
    done = true;
    co_return;
}

TEST(TaskLifetime, UnlaunchedTaskFrameDestroyed) {
    std::atomic<int> live{0};
    {
        auto task = probe_in_param(LiveCounter{&live});
        EXPECT_EQ(live.load(), 1); // 参数副本已进帧
        // 不 launch,Task 析构应销毁帧
    }
    EXPECT_EQ(live.load(), 0);
}

TEST(TaskLifetime, AwaitedChildFrameDestroyed) {
    // awaiter 取完结果后负责销毁 child 帧(挂在 final_suspend 的帧)
    std::atomic<int> live{0};
    std::atomic<bool> done{false};
    parent_awaits_probe_child(&live, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    // child 已完成且帧已销毁;parent 可能还在 WORKER 上收尾,等它
    ASSERT_TRUE(pump_until([](void *p) { return static_cast<std::atomic<int> *>(p)->load() == 0; }, &live));
    EXPECT_EQ(live.load(), 0);
}

TEST(TaskLifetime, DetachedFrameSelfDestroyed) {
    std::atomic<int> live{0};
    std::atomic<bool> done{false};
    detached_probe(&live, done).launch();
    ASSERT_TRUE(pump_until_flag(done));
    ASSERT_TRUE(pump_until([](void *p) { return static_cast<std::atomic<int> *>(p)->load() == 0; }, &live));
    EXPECT_EQ(live.load(), 0);
}

TEST(TaskLifetime, MoveTransfersOwnership) {
    std::atomic<int> live{0};
    {
        auto task = probe_in_param(LiveCounter{&live});
        auto moved = std::move(task);
        EXPECT_EQ(live.load(), 1);
        Task<void> assigned = probe_in_param(LiveCounter{&live});
        EXPECT_EQ(live.load(), 2);
        assigned = std::move(moved); // 移动赋值:销毁 assigned 原帧,接管 moved 的帧
        EXPECT_EQ(live.load(), 1);
    }
    EXPECT_EQ(live.load(), 0);
}
