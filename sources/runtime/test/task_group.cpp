#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "core/Task.h"
#include "core/TaskGroup.h"
#include "core/ThreadPool.h"
#include "core/ThreadUtils.h"

using namespace Goonya;
using namespace std::chrono_literals;

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

// 注意:spawn 在当前线程启动任务,需要 WORKER 执行的任务必须自己先 switch_thread
static Task<void> work_on_worker(std::atomic<int> &counter, ThreadType &last_thread) {
    co_await switch_thread(ThreadType::WORKER);
    counter.fetch_add(1);
    last_thread = current_thread_type;
    co_return;
}

static Task<void> throw_on_worker() {
    co_await switch_thread(ThreadType::WORKER);
    throw std::runtime_error{"inner boom"};
}

static Task<void> slow_flag(std::atomic<bool> &flag) {
    co_await switch_thread(ThreadType::WORKER);
    std::this_thread::sleep_for(50ms);
    flag = true;
    co_return;
}

static Task<void> no_switch(ThreadType &body_thread) {
    body_thread = current_thread_type;
    co_return;
}

// ---------------- TaskGroup ----------------

TEST(TaskGroup, SpawnStartsOnCallingThread) {
    // 行为锁定:wrap 不切换线程,任务体直接运行在 spawn 的调用线程上。
    // 需要后台执行的任务必须以 co_await switch_thread(...) 开头
    TaskGroup group;
    ThreadType body_thread = ThreadType::UNKNOWN;
    group.spawn(no_switch(body_thread));
    group.join();
    EXPECT_EQ(body_thread, ThreadType::RENDER);
}

TEST(TaskGroup, JoinWaitsAllTasks) {
    TaskGroup group;
    std::atomic<int> counter{0};
    ThreadType last_thread = ThreadType::UNKNOWN;
    for (int i = 0; i < 100; i++) {
        group.spawn(work_on_worker(counter, last_thread));
    }
    group.join(); // join 返回时,所有任务必须已完成
    EXPECT_EQ(counter.load(), 100);
    EXPECT_EQ(last_thread, ThreadType::WORKER);
}

TEST(TaskGroup, InnerExceptionDoesNotHangJoin) {
    // 内层协程抛异常:Guard 析构仍减计数,join 不能挂死
    // (会输出一条"协程内被丢弃的异常"日志,属预期)
    TaskGroup group;
    group.spawn(throw_on_worker());
    group.join();
    SUCCEED(); // 到达这里即未挂死、未 terminate
}

TEST(TaskGroup, DestructorJoins) {
    std::atomic<bool> flag{false};
    {
        TaskGroup group;
        group.spawn(slow_flag(flag));
        EXPECT_FALSE(flag.load()); // 任务在 worker 上 sleep,此刻必然未完成
    } // ~TaskGroup 阻塞直到任务完成
    EXPECT_TRUE(flag.load());
}
