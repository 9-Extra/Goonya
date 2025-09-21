#pragma once

#include "core/ThreadUtils.h" // IWYU pragma: keep

#include <cassert>

#define IS_RENDER_THREAD() (::Goonya::current_thread_type == ::Goonya::ThreadType::RENDER)

#define ASSERT_RENDER_THREAD() assert(IS_RENDER_THREAD())