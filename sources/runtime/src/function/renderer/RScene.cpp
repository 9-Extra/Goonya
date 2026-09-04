#include "RScene.h"
#include "core/cgmath/matrix.h"
#include "function/renderer/PipelineLayout.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "runtime/GAssert.h"
#include <optional>
#include <utility>

namespace Goonya {

RScene::~RScene() {
    // 可渲染对象必须先于场景销毁(由World/LevelRenderer等所有者的析构顺序保证)
    GN_ASSERT(renderables.empty());
    for (RCamera *camera : cameras) {
        delete camera;
    }
}

// 法线矩阵 = 模型矩阵上3x3的逆转置; 上传时的转置(见model_matrix)与逆转置的转置抵消, 因此直接上传逆矩阵
// 奇异矩阵(如零缩放)时退回单位阵
static Matrix4f compute_normal_matrix(const Matrix4f &model_matrix) noexcept {
    if (std::optional<Matrix3f> inverse = model_matrix.to_matrix3().inverse()) {
        return Matrix4f{*inverse};
    }
    return Matrix4f::identity();
}

void RScene::commit() noexcept {
    for (auto &&renderable : renderables) {
        if (renderable.dirty == RenderableDirty::None) {
            continue;
        }

        // 两条路径都重写uniform, 128字节的成本换来更新逻辑的折叠
        PerObjectData per_object_data{
            .model_matrix = renderable.model_matrix.transpose(),
            .normal_matrix = compute_normal_matrix(renderable.model_matrix),
        };
        renderable.per_object_uniform->write(&per_object_data, BufferMapOption::WRITE_DISCARD);

        if (renderable.dirty == RenderableDirty::Structure) {
            remove_mesh_instances(renderable);
            if (!renderable.hidden && renderable.mesh) {
                Vector3f position = renderable.model_matrix.resolve_position();
                for (size_t i = 0; i < renderable.mesh->get_submeshes().size(); ++i) {
                    const SubMesh &submesh = renderable.mesh->get_submeshes()[i];
                    if (submesh.index_count == 0) {
                        continue; // 没有顶点，跳过
                    }
                    // 材质与submesh按下标一一对应, 缺省时由Pipeline使用默认材质
                    Ref<Material> material = i < renderable.materials.size() ? renderable.materials[i] : nullptr;
                    const BoundingBox &local_bounds =
                        renderable.has_bounds_override ? renderable.bounds_override : submesh.aabb;
                    renderable.instances.push_back(
                        instances.emplace(renderable.mesh, std::move(material), submesh, renderable.per_object_uniform,
                                          position, local_bounds.transformed(renderable.model_matrix)));
                }
            }
        } else { // RenderableDirty::Dynamic
            Vector3f position = renderable.model_matrix.resolve_position();
            for (Handle<Instance> h_instance : renderable.instances) {
                Instance &instance = instances[h_instance];
                instance.position = position;
                const BoundingBox &local_bounds =
                    renderable.has_bounds_override ? renderable.bounds_override : instance.submesh.aabb;
                instance.transformed_bbox = local_bounds.transformed(renderable.model_matrix);
            }
        }

        renderable.dirty = RenderableDirty::None;
    }
}

Handle<RRenderable> RScene::create_renderable() noexcept {
    Handle<RRenderable> handle = renderables.emplace();
    renderables[handle].per_object_uniform = create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerObjectData));
    return handle;
}

void RScene::destroy_renderable(Handle<RRenderable> handle) noexcept {
    RRenderable *renderable = renderables.get_or_null(handle);
    if (!renderable) {
        return;
    }
    remove_mesh_instances(*renderable);
    renderables.remove(handle);
}

void RScene::set_mesh(Handle<RRenderable> handle, Ref<Mesh> mesh) noexcept {
    RRenderable &renderable = renderables[handle];
    renderable.mesh = std::move(mesh);
    mark_dirty(renderable, RenderableDirty::Structure);
}

void RScene::set_materials(Handle<RRenderable> handle, std::span<const Ref<Material>> materials) noexcept {
    RRenderable &renderable = renderables[handle];
    renderable.materials.assign(materials.begin(), materials.end());
    mark_dirty(renderable, RenderableDirty::Structure);
}

void RScene::set_hidden(Handle<RRenderable> handle, bool hidden) noexcept {
    RRenderable &renderable = renderables[handle];
    if (renderable.hidden == hidden) {
        return;
    }
    renderable.hidden = hidden;
    mark_dirty(renderable, RenderableDirty::Structure);
}

void RScene::set_transform(Handle<RRenderable> handle, const Matrix4f &model_matrix) noexcept {
    RRenderable &renderable = renderables[handle];
    renderable.model_matrix = model_matrix;
    mark_dirty(renderable, RenderableDirty::Dynamic);
}

void RScene::set_bounds_override(Handle<RRenderable> handle, BoundingBox bounds) noexcept {
    RRenderable &renderable = renderables[handle];
    renderable.bounds_override = bounds;
    renderable.has_bounds_override = true;
    mark_dirty(renderable, RenderableDirty::Dynamic);
}

void RScene::clear_bounds_override(Handle<RRenderable> handle) noexcept {
    RRenderable &renderable = renderables[handle];
    renderable.has_bounds_override = false;
    mark_dirty(renderable, RenderableDirty::Dynamic);
}

void RScene::remove_mesh_instances(RRenderable &renderable) noexcept {
    for (Handle<Instance> h_instance : renderable.instances) {
        instances.remove(h_instance);
    }
    renderable.instances.clear();
}

} // namespace Goonya
