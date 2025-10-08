#pragma once

#include "core/cgmath.h"
#include "core/plf_colony.h"
#include "function/renderer/RenderAspect.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

namespace Goonya::Graphics {
// 渲染mesh的组件，可以渲染出物体
class CpntSkybox : public Component {

private:
    Ref<Material> skybox_material;
    bool ignore_range;
    BoundingBox bbox;

    plf::colony<Skybox>::iterator skybox_proxy;

public:
    explicit CpntSkybox(const Ref<Material> &skybox_material, bool ignore_range = true,
                        const BoundingBox &bbox = BoundingBox())
        : skybox_material(skybox_material), ignore_range(ignore_range), bbox(bbox) {}
    void on_register() override {
        assert(get_owner() != nullptr);
        Vector3f pos = get_owner()->get_world_model_matrix().resolve_translate();

        Skybox skybox{
            skybox_material,
            ignore_range,
            bbox.offset(pos)
        };

        skybox_proxy = world.main_scene()->skyboxs.emplace(std::move(skybox));
    }

    void on_unregister() override { world.main_scene()->skyboxs.erase(skybox_proxy); }

    void on_tick() override {}
};
} // namespace Goonya::Graphics