#include "Graphics.h"

#include "opengl/OpenGLAPI.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

std::unique_ptr<GraphicsAPI> graphics_api;

void initialize(GraphicsAPIType api_type) {
    switch (api_type) {
        case GraphicsAPIType::NONE: throw RuntimeError("不行");
        case GraphicsAPIType::OPENGL: {graphics_api.reset(new OpenGLGraphicsAPI); break;};
        default:
            throw RuntimeError("不支持的API类型");
    }
}

void drop(){
    graphics_api.reset();
}

} // namespace Graphics
} // namespace Goonya