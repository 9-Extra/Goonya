#include <iostream>

#include "graphics.h"
#include "opengl_utils.h"

namespace Goonya {
namespace Graphics {

void setup_opengl() {
    GLenum err = gladLoadGL();
    if (err != GL_TRUE) {
        std::cerr << "Error: " << err << std::endl;
    }

    const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << vendorName << ": " << version << std::endl;

    // if (!GL_EXT_gpu_shader4) {
    //     std::cerr << "不兼容拓展" << std::endl;
    // }

    glClearColor(0.0, 0.0, 0.0, 0.0);

    checkError();
}

void initialize() {
    setup_opengl();
}

} // namespace Graphics
} // namespace Goonya