#pragma once

#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/vector.h"
#include "core/sparse_set.h"
#include "function/renderer/Camera.h"
#include "function/renderer/Material.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLTexture.h"

#include <cstddef>
#include <vector>

namespace Goonya {

struct REnvironment {
    BoundingBox aabb;
    bool is_infinite = false;

    Ref<GLTexture> environment_map;
    Ref<GLTexture> skybox;
    Vector3f skybox_permutation = Vector3f{1.0f, 1.0f, 1.0f};
    Vector3f ambient_light = Vector3f{0.1f, 0.1f, 0.1f};
    Vector3f clear_color = Vector3f{0.0f, 0.0f, 0.0f};

    bool enable_fog = false;
    float fog_density = 1.0f;
    float fog_min_distance = 30.0f;
};

struct RLight {
    Vector3f position = Vector3f{0.0f, 0.0f, 0.0f};
    Vector3f linear_color = Vector3f{1.0f, 1.0f, 1.0f};
    float intensity = 10.0f;
    Vector3f direction = Vector3f{0.0f, -1.0f, 0.0f};
};

struct Instance {
    Ref<GLMesh> mesh;
    Ref<Material> material;
    SubMesh submesh;

    Ref<GLBuffer> per_object_uniform;
    BoundingBox transformed_bbox;
};

class RCamera;
class Pipeline;
class IMeshRenderable;

class RScene final {
private:
    friend class Pipeline;

    std::vector<IMeshRenderable *> meshes;
    std::vector<size_t> mesh_free_list;
    std::vector<IMeshRenderable *> meshes_to_update;

    SparseSet<Instance> instances;

    SparseSet<REnvironment> environments;
    SparseSet<RLight> lights;
    std::vector<RCamera *> cameras;

public:
    RScene() = default;
    ~RScene();
    void do_pending_updates() noexcept;

    void register_mesh(IMeshRenderable *mesh) noexcept;
    void enqueue_mesh_update(IMeshRenderable *mesh) noexcept;
    void unregister_mesh(IMeshRenderable *mesh) noexcept;

    Handle<REnvironment> add_environment() noexcept { return environments.emplace(); }
    void drop_environment(Handle<REnvironment> handle) noexcept { environments.remove(handle); }
    void set_environment_aabb(Handle<REnvironment> handle, BoundingBox aabb, bool is_infinite = false) noexcept {
        environments[handle].aabb = aabb;
        environments[handle].is_infinite = is_infinite;
    }
    void set_environment_map(Handle<REnvironment> handle, const Ref<GLTexture> &environment_map) noexcept {
        environments[handle].environment_map = environment_map;
    }
    void set_environment_skybox(Handle<REnvironment> handle, const Ref<GLTexture> &skybox,
                                Vector3f color_permutation) noexcept {
        environments[handle].skybox = skybox;
        environments[handle].skybox_permutation = color_permutation;
    }
    void set_environment_clear_color(Handle<REnvironment> handle, Vector3f clear_color) noexcept {
        environments[handle].clear_color = clear_color;
    }

    void set_environment_fog(Handle<REnvironment> handle, bool enable, float density, float min_distance) noexcept {
        environments[handle].enable_fog = enable;
        environments[handle].fog_density = density;
        environments[handle].fog_min_distance = min_distance;
    }

    Handle<RLight> add_light() noexcept { return lights.emplace(); }
    void drop_light(Handle<RLight> light) noexcept { lights.remove(light); }

    void set_light_position(Handle<RLight> light, Vector3f position) noexcept { lights[light].position = position; }
    void set_light_linear_color(Handle<RLight> light, Vector3f linear_color, float intensity) noexcept {
        lights[light].linear_color = linear_color;
        lights[light].intensity = intensity;
    }
    void set_light_direction(Handle<RLight> light, Vector3f direction) noexcept { lights[light].direction = direction; }

    RCamera *add_camera() noexcept {
        cameras.push_back(new RCamera());
        return cameras.back();
    }
    void drop_camera(RCamera *camera) noexcept {
        cameras.erase(std::remove(cameras.begin(), cameras.end(), camera), cameras.end());
        delete camera;
    }

private:
    void gather_mesh_instances(IMeshRenderable *mesh) noexcept;
    void remove_mesh_instances(IMeshRenderable *mesh) noexcept;
    void update_mesh(IMeshRenderable *mesh) noexcept;
};

} // namespace Goonya