#pragma once

#include <glad/glad.h>

#include "runtime/log/Log.h"

inline void _check_error(const char* file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR("GL error 0x{}: At: {}:{}", error, file, line);
    }
}

#ifdef NDEBUG
#define checkError()
#else
#define checkError() _check_error(__FILE__, __LINE__)
#endif // !NDEBUG