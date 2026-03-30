#pragma once

#include "core/cgmath/aabb.h"
#include "core/cgmath/cgmath.h"
#include "core/cgmath/vector.h"
#include "core/sparse_set.h"
#include "function/renderer/RScene.h"
#include "function/world/GObject.h"
#include "function/world/World.h"

namespace Goonya {
// 渲染mesh的组件，可以渲染出物体
class CpntSkybox : public Component {
private:
    RScene *scene = nullptr;
    Ref<GLTexture> skybox;
    Ref<GLTexture> env_map;
    bool ignore_range;
    BoundingBox bbox;

    Handle<REnvironment> h_environment;

public:
    explicit CpntSkybox(const Ref<GLTexture> &skybox, const Ref<GLTexture> &env_map, bool ignore_range = true,
                        const BoundingBox &bbox = BoundingBox())
        : skybox(skybox), env_map(env_map), ignore_range(ignore_range), bbox(bbox) {}

    std::unique_ptr<Component> clone() const override {
        return std::make_unique<CpntSkybox>(skybox, env_map, ignore_range, bbox);
    }

    void on_register() override {
        GN_ASSERT(get_owner() != nullptr);
        Vector3f pos = get_owner()->get_world_model_matrix().resolve_position();
        scene = get_owner()->get_world()->get_scene();
        h_environment = scene->add_environment();
        scene->set_environment_aabb(h_environment, bbox.offset(pos), ignore_range);
        scene->set_environment_skybox(h_environment, skybox, Vector3f{1, 1, 1});
        scene->set_environment_map(h_environment, env_map);
    }

    void on_unregister() override {
        scene->drop_environment(h_environment);
        scene = nullptr;
    }

    void on_update(ComponentUpdateFlag flag) override {
        if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
            Vector3f position = get_owner()->get_world_model_matrix().resolve_position();
            scene->set_environment_aabb(h_environment, bbox.offset(position));
        }
    }
};
} // namespace Goonya
