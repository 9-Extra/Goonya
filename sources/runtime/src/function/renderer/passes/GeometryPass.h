#pragma once

#include "function/renderer/passes/Passes.h"

namespace Goonya {

class GeometryPass : public Pass {
private:
    Ref<GLBuffer> per_frame_uniform; // 用于一般渲染每帧变化的数据
public:
    GeometryPass();

    void run(PassRenderInfo& info) override;
};

} // namespace Goonya
