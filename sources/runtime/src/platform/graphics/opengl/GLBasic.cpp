#include "GLBasic.h"

#include "core/log/Log.h"

namespace Goonya {
namespace Graphics {

void _opengl_check_error(const char *file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::string_view desciption;
        switch (error) {
        case GL_INVALID_ENUM: {
            desciption = "GL_INVALID_ENUM";
            break;
        }
        case GL_INVALID_OPERATION: {
            desciption = "GL_INVALID_OPERATION";
            break;
        }
        case GL_INVALID_VALUE: {
            desciption = "GL_INVALID_VALUE";
            break;
        }
        default: {
            static std::string num_error;
            num_error = std::format("GL error 0x{}", error);
            desciption = num_error;
            break;
        }
        }
        LOG_ERROR("{}: At: {}:{}", desciption, file, line);
    }
}

} // namespace Graphics
} // namespace Goonya