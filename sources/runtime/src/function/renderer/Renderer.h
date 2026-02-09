#pragma once

#include "function/renderer/RScene.h"

#include <memory>
#include <vector>

namespace Goonya {

class Pipeline;

// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    bool draw_bloom = true;

private:
    friend class Pipeline;
    std::unique_ptr<Pipeline> render_pipeline;
    std::vector<RScene *> scenes;

public:
    RScene *create_scene() noexcept;
    void drop_scene(RScene *scene) noexcept;

    void init();
    void render();
    void clear();
};

extern Renderer renderer;
} // namespace Goonya
