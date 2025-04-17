#include "Graphics.h"

#include <memory>

#include "opengl/OpenGLAPI.h"
#include "runtime/GoonyaException.h"

namespace Goonya::Graphics {

std::unique_ptr<GraphicsAPI> graphics_api;

void initialize(GraphicsAPIType api_type) {
    switch (api_type) {
    case GraphicsAPIType::NONE:
        throw RuntimeError("不行");
    case GraphicsAPIType::OPENGL: {
        graphics_api = std::make_unique<OpenGLGraphicsAPI>();
        break;
    };
    default:
        throw RuntimeError("不支持的API类型");
    }
}

void drop() { graphics_api.reset(); }

} // namespace Goonya::Graphics
