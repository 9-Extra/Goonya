#pragma once

#include <spdlog/logger.h>
#include <thread>

#include "opengl/OpenGLAPI.h" // IWYU: pragma export

namespace Goonya {

class OpenGLGraphicsAPI;

extern OpenGLGraphicsAPI GL;
extern std::thread render_thread;

} // namespace Goonya
