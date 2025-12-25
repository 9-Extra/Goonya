#include "GLBasic.h"

#include "OpenGLAPI.h"
#include "platform/graphics/Graphics.h"
#include <spdlog/common.h>
#include <string_view>

namespace Goonya {

void APIENTRY _opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                     const GLchar *message, const void *userParam) {
    std::string_view source_name;
    switch (source) {
    case GL_DEBUG_SOURCE_API: {
        source_name = "";
        break;
    }
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
        source_name = "Window System: ";
        break;
    }
    case GL_DEBUG_SOURCE_SHADER_COMPILER: {
        source_name = "Shader Complier: ";
        break;
    }
    case GL_DEBUG_SOURCE_THIRD_PARTY: {
        source_name = "Third Party: ";
        break;
    }
    case GL_DEBUG_SOURCE_APPLICATION: {
        source_name = "Application: ";
        break;
    }
    default: {
        source_name = "Unknown Source: ";
        break;
    }
    }
    std::string_view type_name;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR: {
        type_name = "Error ";
        break;
    }
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
        type_name = "Deprecated Behavior ";
        break;
    }
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
        type_name = "Undefined Behavior ";
        break;
    }
    case GL_DEBUG_TYPE_PORTABILITY: {
        type_name = "Portability Issus ";
        break;
    }
    case GL_DEBUG_TYPE_PERFORMANCE: {
        type_name = "Performance Issus ";
        break;
    }
    case GL_DEBUG_TYPE_MARKER: {
        type_name = "Marker ";
        break;
    }
    case GL_DEBUG_TYPE_PUSH_GROUP: {
        type_name = "Push Group ";
        break;
    }
    case GL_DEBUG_TYPE_POP_GROUP: {
        type_name = "Pop Group ";
        break;
    }
    default: {
        type_name = "";
        break;
    }
    }

    spdlog::level::level_enum level;
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: {
        level = spdlog::level::err;
        break;
    }
    case GL_DEBUG_SEVERITY_MEDIUM: {
        level = spdlog::level::warn;
        break;
    }
    case GL_DEBUG_SEVERITY_LOW: {
        level = spdlog::level::critical;
        break;
    }
    case GL_DEBUG_SEVERITY_NOTIFICATION: {
        level = spdlog::level::debug;
        break;
    }
    default: {
        level = spdlog::level::trace;
        break;
    }
    }

    std::string_view message_string(message, length);
    if (message_string.ends_with('\n')) {
        message_string.remove_suffix(1); // 不要重复换行
    }

    GL.logger->log(level, "{}{}{}: {}", source_name, type_name, id, message_string);
}

} // namespace Goonya
