#pragma once

namespace Goonya::Graphics {
// Pass 基类
class Pass {
public:
    virtual void run() = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

} // namespace Goonya::Graphics
