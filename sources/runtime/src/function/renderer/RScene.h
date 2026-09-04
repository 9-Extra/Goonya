#pragma once

#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"
#include "core/sparse_set.h"
#include "function/renderer/Camera.h"
#include "function/renderer/Material.h"
#include "function/renderer/Mesh.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLTexture.h"

#include <cstdint>
#include <span>
#include <utility>
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
    Ref<Mesh> mesh;
    Ref<Material> material;
    SubMesh submesh;

    Ref<GLBuffer> per_object_uniform;
    Vector3f position;
    BoundingBox transformed_bbox;
};

class RCamera;
class Pipeline;

// 可渲染对象的脏级别, commit时按级别决定更新方式
enum class RenderableDirty : uint8_t {
    None = 0,
    Dynamic,   // 变换/包围盒变化, 只需要原地更新实例
    Structure, // 网格/材质/显隐变化, 必须重建实例
};

/**
 * @brief 一个可渲染对象的全部渲染侧状态, 集中存储在RScene内部
 * 游戏侧只能通过 RenderableRef 间接操作, 不直接持有本结构
 */
struct RRenderable {
    Ref<Mesh> mesh; //  包含vao，子网格分割，和每个子网格的包围盒
    std::vector<Ref<Material>> materials;
    Matrix4f model_matrix = Matrix4f::identity();

    Ref<GLBuffer> per_object_uniform;
    std::vector<Handle<Instance>> instances;

    BoundingBox bounds_override;
    bool has_bounds_override : 1 = false;
    bool hidden : 1 = false;
    RenderableDirty dirty = RenderableDirty::Structure; // 新建时需要进行一次完整构建
};

class RScene final {
private:
    friend class Pipeline;

    SparseSet<RRenderable> renderables;
    SparseSet<Instance> instances;

    SparseSet<REnvironment> environments;
    SparseSet<RLight> lights;
    std::vector<RCamera *> cameras;

public:
    RScene() = default;
    ~RScene();

    /**
     * @brief 提交所有可渲染对象的挂起更新, 只需要在渲染前调用一次
     */
    void commit() noexcept;

    // ---------- 可渲染对象管理 ----------
    // 一般不直接调用, 请使用 RenderableRef
    Handle<RRenderable> create_renderable() noexcept;
    void destroy_renderable(Handle<RRenderable> handle) noexcept;

    void set_mesh(Handle<RRenderable> handle, Ref<Mesh> mesh) noexcept;
    void set_materials(Handle<RRenderable> handle, std::span<const Ref<Material>> materials) noexcept;
    void set_hidden(Handle<RRenderable> handle, bool hidden = true) noexcept;
    void set_transform(Handle<RRenderable> handle, const Matrix4f &model_matrix) noexcept;
    void set_bounds_override(Handle<RRenderable> handle, BoundingBox bounds) noexcept;
    void clear_bounds_override(Handle<RRenderable> handle) noexcept;

    // ---------- 环境 ----------
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

    // ---------- 灯光 ----------
    Handle<RLight> add_light() noexcept { return lights.emplace(); }
    void drop_light(Handle<RLight> light) noexcept { lights.remove(light); }

    void set_light_position(Handle<RLight> light, Vector3f position) noexcept { lights[light].position = position; }
    void set_light_linear_color(Handle<RLight> light, Vector3f linear_color, float intensity) noexcept {
        lights[light].linear_color = linear_color;
        lights[light].intensity = intensity;
    }
    void set_light_direction(Handle<RLight> light, Vector3f direction) noexcept { lights[light].direction = direction; }

    // ---------- 相机 ----------
    RCamera *add_camera() noexcept {
        cameras.push_back(new RCamera());
        return cameras.back();
    }
    void drop_camera(RCamera *camera) noexcept {
        cameras.erase(std::remove(cameras.begin(), cameras.end(), camera), cameras.end());
        delete camera;
    }

private:
    // 标记脏级别, 只升级不降级
    static void mark_dirty(RRenderable &renderable, RenderableDirty level) noexcept {
        if (renderable.dirty < level) {
            renderable.dirty = level;
        }
    }
    void remove_mesh_instances(RRenderable &renderable) noexcept;
};

/**
 * @brief 可渲染对象的RAII句柄, 析构时自动从场景中注销
 * @note 必须先于其所属的RScene销毁(由World/LevelRenderer等所有者的析构顺序保证)
 */
class RenderableRef final {
private:
    RScene *scene = nullptr;
    Handle<RRenderable> handle{};

public:
    RenderableRef() = default;
    explicit RenderableRef(RScene &scene) : scene(&scene), handle(scene.create_renderable()) {}
    ~RenderableRef() { reset(); }

    RenderableRef(RenderableRef &&other) noexcept
        : scene(std::exchange(other.scene, nullptr)), handle(std::exchange(other.handle, {})) {}
    RenderableRef &operator=(RenderableRef &&other) noexcept {
        if (this != &other) {
            reset();
            scene = std::exchange(other.scene, nullptr);
            handle = std::exchange(other.handle, {});
        }
        return *this;
    }
    RenderableRef(const RenderableRef &) = delete;
    RenderableRef &operator=(const RenderableRef &) = delete;

    void reset() noexcept {
        if (scene) {
            scene->destroy_renderable(handle);
            scene = nullptr;
        }
    }

    bool is_valid() const noexcept { return scene != nullptr; }
    explicit operator bool() const noexcept { return is_valid(); }

    void set_mesh(Ref<Mesh> mesh) noexcept {
        if (scene) scene->set_mesh(handle, std::move(mesh));
    }
    void set_materials(std::span<const Ref<Material>> materials) noexcept {
        if (scene) scene->set_materials(handle, materials);
    }
    void set_hidden(bool hidden = true) noexcept {
        if (scene) scene->set_hidden(handle, hidden);
    }
    void set_transform(const Matrix4f &model_matrix) noexcept {
        if (scene) scene->set_transform(handle, model_matrix);
    }
    void set_bounds_override(BoundingBox bounds) noexcept {
        if (scene) scene->set_bounds_override(handle, bounds);
    }
    void clear_bounds_override() noexcept {
        if (scene) scene->clear_bounds_override(handle);
    }
};

} // namespace Goonya
