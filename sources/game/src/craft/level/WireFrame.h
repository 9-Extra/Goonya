#pragma once
#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/vector.h"
#include "craft/block/block_model.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RenderScene.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLMesh.h"



namespace Craft {

// 玩家看向的方块线框
class WireFrame {
private:
    Ref<Goonya::Material> material;
    Ref<Goonya::GLMesh> mesh;

    Goonya::MeshRenderProxy *mesh_proxy;

public:
    explicit WireFrame(Goonya::RenderScene &render_scene);
    ~WireFrame() {};

    void draw_at(Goonya::Vector3f pos, const BakedBlockModel& model);

    void hide() {
        mesh_proxy->aabbs[0] = Goonya::BoundingBox{{0, 0, 0}, {0, 0, 0}}; // 不可见
    }
};

} // namespace Craft
