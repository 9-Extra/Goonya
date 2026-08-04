#pragma once

#include "core/RefCount.h"
#include "core/ThreadPool.h"
#include "core/ThreadUtils.h"
#include "runtime/GAssert.h"
#include <cassert>
#include <concepts>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <stop_token>
#include <type_traits>
#include <utility>
#include <variant>

namespace Goonya {

// 任务取消使用异常执行流，在任务取消时抛出
struct TaskCancel {};

/**
 * @brief 获取任务函数F在传入T作为参数时的返回值类型，兼容带std::stop_token和不带的两种情况，兼容T为void的无参数情况
 */
template <typename F, typename T>
struct conditional_result {
private:
    static consteval auto select() {
        if constexpr (std::is_void_v<T>) {
            if constexpr (std::is_invocable_v<F, std::stop_token>) {
                return std::type_identity<std::invoke_result_t<F, std::stop_token>>{};
            } else if constexpr (std::is_invocable_v<F>) {
                return std::type_identity<std::invoke_result_t<F>>{};
            } else {
                static_assert(false, "then 的函数签名不合法：必须能接收上一个 Future 的结果");
            }
        } else {
            if constexpr (std::is_invocable_v<F, std::stop_token, T>) {
                return std::type_identity<std::invoke_result_t<F, std::stop_token, T>>{};
            } else if constexpr (std::is_invocable_v<F, T>) {
                return std::type_identity<std::invoke_result_t<F, T>>{};
            } else {
                static_assert(false, "then 的函数签名不合法：必须能接收上一个 Future 的结果");
            }
        }
    }

public:
    using type = decltype(select())::type;
};

template <typename F, typename T>
using conditional_result_t = typename conditional_result<F, T>::type;

/**
 * @brief 用于Future::then的函数约束
 * 可以选择是否接收std::stop_token，放在前面，必须接收上一个Future的返回值，如果是void则留空
 * 比如上个Future函数返回int，则函数参数必须为(std::stop_token, int)或者(int)
 * @tparam F Lambda表达式类型
 * @tparam ARG 传入的参数类型，可以是void
 */
template <typename F, typename ARG>
concept ChainFunc = requires { typename conditional_result_t<F, ARG>; };

/**
 * @brief 用于Future::async的函数约束
 * 可以选择是否接收std::stop_token，不能有其他输入参数
 * @tparam F Lambda表达式类型
 * @tparam T 返回值类型，需要和Future匹配
 */
template <typename F, typename T>
concept AsyncFunc = std::is_invocable_r_v<T, F> || std::is_invocable_r_v<T, F, std::stop_token>;

inline void cancel_current_task() {
    throw TaskCancel{}; // NOLINT(hicpp-exception-baseclass)
}

/**
 * @brief 创建即立即开始执行的任务对象，支持后继任务
 */
template <typename T>
class Future : public RefCount {
private:
    using ResultType = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    std::variant<std::monostate, ResultType, std::exception_ptr> result;

    std::move_only_function<void()> on_complete;
    std::mutex on_complete_mutex;
    std::condition_variable complete_blocker;
    std::stop_source canceller;
    bool is_taken = false; // 防二次读取
    bool is_completed = false;

public:
    Future(Future &) = delete;
    Future(Future &&) = delete;

    /**
     * @brief 创建一个异步任务立即开始执行
     * @param type 执行任务的线程
     * @param f 任务函数体
     */
    template <typename F>
        requires AsyncFunc<F, T>
    static Ref<Future<T>> async(Goonya::ThreadType type, F f) {
        return async(type, std::stop_source{}, std::move(f));
    }

    /**
     * @brief 创建一个异步任务立即开始执行（使用外部取消源）
     * @param type 执行任务的线程
     * @param canceller 用于取消此任务
     * @param f 任务函数体
     */
    template <typename F>
        requires AsyncFunc<F, T>
    static Ref<Future<T>> async(Goonya::ThreadType type, std::stop_source canceller, F f) {
        assert(type != ThreadType::UNKNOWN);
        Ref<Future<T>> future = empty(std::move(canceller));
        THREAD_POOL.enqueue_thread(
            type, [future, st = future->canceller.get_token(), f = std::move(f)]() mutable { future->execute(f, st); });
        return future;
    }

    /**
     * @brief 初始化一个处于“已完成”状态的任务
     * @param value 任务结果的值
     * @param canceller 用于取消任务的stop_source，已完成任务也可以发起后继任务
     */
    template <typename U>
        requires std::constructible_from<T, U>
    static Ref<Future<T>> completed(U &&value, std::stop_source canceller = {}) {
        auto r = empty(std::move(canceller));
        r->result = std::forward<U>(value);
        r->is_completed = true; // 构造中，不存在竞争
        return r;
    }

    /**
     * @brief 初始化一个处于“已完成”状态的任务（void类型特化）
     */
    static Ref<Future<void>> completed(std::stop_source canceller = {})
        requires std::is_void_v<T>
    {
        auto r = Future<void>::empty(std::move(canceller));
        r->is_completed = true; // 构造中，不存在竞争
        return r;
    }

    /**
     * @brief 注册一个后继任务，接收当前任务的返回值作为输入
     * @param type 在什么线程上执行
     * @param f 任务函数体
     * @return 代表后继任务的Future对象
     */
    template <typename F>
        requires ChainFunc<F, T>
    auto then(Goonya::ThreadType type, F f) -> Ref<Future<conditional_result_t<F, T>>> {
        assert(type != ThreadType::UNKNOWN);
        using R = conditional_result_t<F, T>;

        Ref<Future<R>> future = Future<R>::empty(canceller);
        auto callback = [this, future, type, f = std::move(f)]() mutable {
            if (canceller.stop_requested()) {
                future->result = std::make_exception_ptr(TaskCancel{});
                future->handle_complete();
                return;
            }
            if (result.index() == 2) {
                future->result = std::get<2>(result);
                future->handle_complete();
                return;
            }
            if constexpr (!std::is_void_v<T>) {
                THREAD_POOL.enqueue_thread(type,
                                           [future, r = std::move(std::get<1>(result)), f = std::move(f)]() mutable {
                                               std::stop_token st = future->canceller.get_token();
                                               future->execute(f, st, std::move(r));
                                           });
            } else {
                THREAD_POOL.enqueue_thread(type, [future, f = std::move(f)]() mutable {
                    std::stop_token st = future->canceller.get_token();
                    future->execute(f, st);
                });
            }
        };

        register_on_complete(std::move(callback));
        return future;
    }

    /**
     * @brief 取消整个任务链
     */
    bool cancel() { return canceller.request_stop(); }

    bool is_cancellable() const { return canceller.stop_possible(); }
    bool is_cancelled() const { return canceller.stop_requested(); }

    /**
     * @brief 阻塞直到任务执行完成
     * @note 不要在主线程上等待主线程任务执行完成，会锁死！通常只用在等待工作线程的任务上
     */
    void wait() {
        std::unique_lock<std::mutex> lock(on_complete_mutex);
        complete_blocker.wait(lock, [this]() { return is_completed; });
    }

    /**
     * @brief 阻塞等待后取出任务结果
     * @note 如任务取消，会抛出TaskCancel类型，任务内异常会被忽略，此类型不能被std::exception捕获，需要额外处理
     * @note 已取消的任务也可能正常获取结果，不建议依赖这样的情况
     */
    T take_result() {
        wait();
        GN_ASSERT_MSG(!is_taken, "不能多次取值，then和take_result只能选一个");
        is_taken = true;
        if (result.index() == 2) {
            std::rethrow_exception(std::get<2>(result));
        } else {
            if constexpr (!std::is_void_v<T>) {
                return std::move(std::get<1>(result));
            } else {
                return;
            }
        }
    }

    // 绝对不可以取消在销毁时取消，头部任务外部引用清空 != 整条任务链丢弃
    ~Future() override = default;

private:
    explicit Future(std::stop_source canceller) : canceller(std::move(canceller)) {};

    static Ref<Future<T>> empty(std::stop_source canceller = {}) {
        return Ref<Future<T>>{new Future<T>{std::move(canceller)}};
    }

    template <typename R>
    friend class Future;

    template <typename E>
    void register_on_complete(E &&callback) {
        bool fire = false;
        {
            std::lock_guard lock{on_complete_mutex};
            GN_ASSERT_MSG(!on_complete, "不能重复注册后继任务");
            GN_ASSERT_MSG(!is_taken, "then和take_result只能选一个");
            is_taken = true;
            if (is_completed) {
                fire = true; // 如果任务已结束，就直接由当前线程发起
            } else {
                on_complete = std::forward<E>(callback); // 否则由执行任务的线程发起
            }
        }
        if (fire) {
            callback();
        }
    }

    void handle_complete() {
        std::move_only_function<void()> t;
        {
            std::lock_guard lock{on_complete_mutex};
            is_completed = true;
            t = std::move(on_complete);
        }
        complete_blocker.notify_all();

        if (t) {
            t();
        }
    }

    template <typename F, typename... ARGS>
    void execute(F &f, std::stop_token &st, ARGS &&...args) {
        if (st.stop_requested()) {
            result = std::make_exception_ptr(TaskCancel{});
            handle_complete();
            return;
        }
        try {
            if constexpr (!std::is_void_v<T>) {
                if constexpr (std::is_invocable_v<F, std::stop_token, ARGS...>) {
                    result = f(st, std::forward<ARGS>(args)...);
                } else {
                    result = f(std::forward<ARGS>(args)...);
                }
            } else {
                if constexpr (std::is_invocable_v<F, std::stop_token, ARGS...>) {
                    f(st, std::forward<ARGS>(args)...);
                } else {
                    f(std::forward<ARGS>(args)...);
                }
            }
        } catch (...) {
            result = std::current_exception();
        }
        handle_complete();
    }
};
} // namespace Goonya