#pragma once

#include "core/RefCount.h"
#include "core/cgmath/vector.h"
#include "craft/block/block_model.h"
#include "function/renderer/Material.h"
#include "function/renderer/Mesh.h"
#include "function/renderer/RScene.h"

#include <vector>

namespace Craft {

// 玩家看向的方块线框
class WireFrame {
private:
    Goonya::MeshDataArrays mesh_data;
    Goonya::RenderableRef renderable;
    Ref<Goonya::Mesh> mesh;
    std::vector<Ref<Goonya::Material>> materials;

public:
    explicit WireFrame(Goonya::RScene *scene);
    ~WireFrame() {};

    void draw_at(Goonya::Vector3f pos, const BakedBlockModel &model);
    void set_hidden(bool hidden = true) { renderable.set_hidden(hidden); }
};

} // namespace Craft
