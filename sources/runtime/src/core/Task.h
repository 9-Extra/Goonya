#pragma once

#include "core/ThreadPool.h"
#include "core/ThreadUtils.h"
#include "core/format_exception.h"
#include "core/log/Log.h"
#include "runtime/GAssert.h"

#include <concepts>
#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

namespace Goonya {

template <typename T>
class Task {
private:
    struct FinalAwaiter {
        // 协程结束时挂起，保留返回值
        bool await_ready() const noexcept { return false; }

        template <typename P>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<P> cf) const noexcept {
            static_assert(std::derived_from<P, PromiseBase>); // 只有Task<T>才会调用FinalAwaiter（在final_suspend()中）
            auto &p = cf.promise();
            if (p.next) {
                return p.next; // 在当前线程上继续下一个任务
            }
            if (p.result.index() == 2) {
                // 结果静默丢弃，但异常还是输出到日志
                try {
                    std::rethrow_exception(std::get<2>(p.result));
                } catch (const std::exception &e) {
                    LOG_ERROR("协程内被丢弃的异常：{}", format_exception(e));
                } catch (...) {
                    LOG_ERROR("协程内被丢弃的异常：未知类型");
                }
            }
            cf.destroy();                 // 否则没有任何对象会继续访问结果，销毁
            return std::noop_coroutine(); // 等用于返回void，会退出resume()函数
        }
        void await_resume() const noexcept {}
    };

    struct PromiseBase {
        std::variant<std::monostate, std::conditional_t<std::is_void_v<T>, std::monostate, T>, std::exception_ptr>
            result;
        std::coroutine_handle<> next; // 后继任务可以是任何类型的协程

        Task<T> get_return_object();
        // lazy
        std::suspend_always initial_suspend() const noexcept { return {}; }
        void unhandled_exception() { result = std::current_exception(); }
        FinalAwaiter final_suspend() const noexcept { return {}; }
    };
    struct PromiseValue : public PromiseBase {
        // value的类型声明只是为了通过编译，实际上T为void时使用PromiseVoid
        void return_value(std::conditional_t<std::is_void_v<T>, std::monostate, T> value) { this->result = value; }
    };

    struct PromiseVoid : public PromiseBase {
        void return_void() {}
    };

public:
    using promise_type = std::conditional_t<std::is_void_v<T>, PromiseVoid, PromiseValue>;

private:
    struct TaskAwaiter {
        // 挂起旧任务执行新任务，执行完后切换回来（见FinalAwaiter::await_suspend）
        std::coroutine_handle<promise_type> handle;

        bool await_ready() const noexcept {
            // 新任务永远未完成
            return false;
        }
        // 类型擦除的std::coroutine_handle<>将允许任何类型的协程调用co_await Task<T>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> cf) const noexcept {
            GN_ASSERT_MSG(handle, "一个任务不能被 co_await 两次");
            handle.promise().next = cf; // 下一个切换的任务是旧任务，也就是当前上下文的任务
            return handle;              // 切换到新任务
        }
        // 子协程的异常从这里rethrow给调用方
        T await_resume() const {
            // RAII:无论正常取值还是rethrow,离开作用域时销毁已完成的子协程帧（此时它挂在final_suspend）
            struct FrameGuard {
                std::coroutine_handle<> h;
                ~FrameGuard() { h.destroy(); }
            } guard{handle};

            auto &r = handle.promise().result;
            if (r.index() == 2) {
                std::rethrow_exception(std::get<2>(r)); // exception_ptr按值拷贝传入,帧销毁不影响异常对象
            }
            if constexpr (std::is_void_v<T>) {
                return;
            } else {
                return std::move(std::get<1>(r)); // 返回值先move出来,guard析构在return之后
            }
        }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> handle) : handle(handle) {}

public:
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
    Task &operator=(Task &&other) noexcept {
        if (&other == this) return *this;
        if (handle) {
            handle.destroy();
        }
        handle = std::exchange(other.handle, nullptr);
        return *this;
    }
    /**
     * @brief 在**当前线程**上立即开始执行任务
     */
    void launch() && {
        GN_ASSERT_MSG(handle, "一个任务不能多次启动");
        std::exchange(handle, nullptr).resume(); // 开始运行协程
    }

    /**
     * @brief 切换到新任务执行，完成后切回
     * @note 切换回来之后可能在任意线程上，必要时switch_thread
     */
    TaskAwaiter operator co_await() && { return TaskAwaiter{std::exchange(handle, nullptr)}; }

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }
};

template <typename T>
Task<T> Task<T>::PromiseBase::get_return_object() {
    // 注意:必须用from_promise(promise引用),from_address要的是帧起始地址,promise只是帧内的子对象,两者不同
    return Task<T>{std::coroutine_handle<promise_type>::from_promise(*static_cast<Task<T>::promise_type *>(this))};
}

struct SwitchThread {
    ThreadType type;
    bool delay; // 是否拖延到帧尾。对WORKER没有意义，但保留推到另一线程的操作
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    bool await_ready() const noexcept { return current_thread_type == type && !delay; }
    void await_suspend(std::coroutine_handle<> h) const noexcept {
        THREAD_POOL.enqueue_thread(type, [h]() { h.resume(); });
    }
    void await_resume() const noexcept {}
};

inline SwitchThread switch_thread(ThreadType type, bool delay = false) { return SwitchThread{type, delay}; }

} // namespace Goonya