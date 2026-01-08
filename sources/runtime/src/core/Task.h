#pragma once

#include "core/RefCount.h"
#include <exception>

template <typename T>
union MaybeUninit {
    T storage;
};

/**
 * @brief 类似与std::function的封装器，但是支持取消，后继任务
 *
 */
template <typename T>
class Task : public RefCount {
    enum class TaskState {
        BUILDING,
        PENDING,
        SCHEDULED,
        RUNNING,

        CANCELED,
        COMPLETED, // result or exception
    };

private:
    TaskState state;
    MaybeUninit<T> result;
    std::exception_ptr exception;

public:
    Task() {}
};