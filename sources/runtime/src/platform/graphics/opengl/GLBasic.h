#pragma once

#include <glad/glad.h>

#include "core/log/Log.h"

#ifdef NDEBUG
#define opengl_debug_check_error()
#else
#define opengl_debug_check_error() _opengl_check_error(__FILE__, __LINE__)
#endif // !NDEBUG

#define opengl_check_error() _opengl_check_error(__FILE__, __LINE__)

namespace Goonya {
namespace Graphics{

inline void _opengl_check_error(const char *file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR("GL error 0x{}: At: {}:{}", error, file, line);
    }
}

}
}