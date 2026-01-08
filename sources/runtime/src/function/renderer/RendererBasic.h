#pragma once

#include "core/ThreadUtils.h" // IWYU pragma: keep

#define IS_RENDER_THREAD() (::Goonya::current_thread_type == ::Goonya::ThreadType::RENDER)

#define ASSERT_RENDER_THREAD() GN_ASSERT(IS_RENDER_THREAD())