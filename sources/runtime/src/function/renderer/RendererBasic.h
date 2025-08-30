#pragma once

#include "core/ThreadUtils.h" // IWYU pragma: keep

#include <cassert>

#define IS_RENDER_THREAD() (current_thread_type == ThreadType::RENDER)

#define ASSERT_RENDER_THREAD() assert(IS_RENDER_THREAD())