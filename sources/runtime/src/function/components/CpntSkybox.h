#pragma once

#include "core/cgmath/aabb.h"
#include "core/cgmath/cgmath.h"
#include "core/plf_colony.h"
#include "function/renderer/RenderAspect.h"
#include "function/renderer/RenderScene.h"
#include "function/renderer/Renderer.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

namespace Goonya {
// 渲染mesh的组件，可以渲染出物体
class CpntSkybox : public Component {

private:
    Ref<GLTexture> env_map;
    Ref<Material> skybox_material;
    bool ignore_range;
    BoundingBox bbox;

    plf::colony<Skybox>::iterator skybox_proxy;

public:
    explicit CpntSkybox(const Ref<Material> &skybox_material, bool ignore_range = true,
                        const BoundingBox &bbox = BoundingBox())
        : skybox_material(skybox_material), ignore_range(ignore_range), bbox(bbox) {}
    void on_register() override {
        GN_ASSERT(get_owner() != nullptr);
        Vector3f pos = get_owner()->get_world_model_matrix().resolve_position();

        Skybox skybox{env_map, skybox_material, ignore_range, bbox.offset(pos)};

        RenderScene &scene = renderer.scene_set[get_owner()->get_world()->main_scene()];
        skybox_proxy = scene.skyboxs.emplace(std::move(skybox));
        // scene->skyboxs.erase(skybox_proxy);
    }

    void on_unregister() override {
        RenderScene &scene = renderer.scene_set[get_owner()->get_world()->main_scene()];
        scene.skyboxs.erase(skybox_proxy);
    }

    void on_update(ComponentUpdateFlag flag) override {
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            Vector3f position = get_owner()->get_world_model_matrix().resolve_position();
            skybox_proxy->bbox = bbox.offset(position);
        }
    }
};
} // namespace Goonya
