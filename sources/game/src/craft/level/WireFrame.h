#pragma once

#include "core/cgmath/vector.h"
#include "craft/block/block_model.h"
#include "function/renderer/IMeshRenderable.h"
#include "function/renderer/RScene.h"

namespace Craft {

// 玩家看向的方块线框
class WireFrame : public Goonya::IMeshRenderable {
public:
    explicit WireFrame(Goonya::RScene *scene);
    ~WireFrame() {};

    void draw_at(Goonya::Vector3f pos, const BakedBlockModel &model);
};

} // namespace Craft
