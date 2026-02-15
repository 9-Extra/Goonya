#include "RScene.h"
#include "core/cgmath/matrix.h"
#include "function/renderer/IMeshRenderable.h"
#include "function/renderer/PipelineLayout.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "runtime/GAssert.h"
#include <cstddef>
#include <vector>

namespace Goonya {

RScene::~RScene() {
    for (IMeshRenderable *mesh : meshes) {
        if (mesh) {
            mesh->scene = nullptr;
            mesh->index_in_scene = 0;
            mesh->pending_update_index = IMeshRenderable::InvalidIndex;
        }
    }
    for (RCamera *camera : cameras) {
        delete camera;
    }
}

void RScene::do_pending_updates() noexcept {
    for (IMeshRenderable *mesh : meshes_to_update) {
        if (!mesh) {
            continue; // 被删除了
        }
        update_mesh(mesh);
        mesh->pending_update_index = IMeshRenderable::InvalidIndex;
    }
    meshes_to_update.clear();
}

void RScene::register_mesh(IMeshRenderable *mesh) noexcept {
    GN_ASSERT(mesh);
    if (mesh->scene != nullptr) {
        if (mesh->scene == this) {
            return;
        } else {
            mesh->scene->unregister_mesh(mesh);
        }
    }
    mesh->scene = this;

    if (!mesh_free_list.empty()) [[likely]] {
        size_t index = mesh_free_list.back();
        mesh_free_list.pop_back();
        GN_ASSERT(meshes[index] == nullptr);
        meshes[index] = mesh;
        mesh->index_in_scene = index;
    } else {
        size_t index = meshes.size();
        meshes.emplace_back(mesh);
        mesh->index_in_scene = index;
    }

    mesh->per_object_uniform = create_ref<GLBuffer>(BufferType::MODIFIABLE, sizeof(PerObjectData));
    mesh->mark_dirty(IMeshRenderable::DirtyBit::Init);
}

void RScene::unregister_mesh(IMeshRenderable *mesh) noexcept {
    if (!mesh->scene) {
        return;
    }
    GN_ASSERT(mesh->scene == this);

    remove_mesh_instances(mesh);

    mesh->per_object_uniform.reset();

    mesh_free_list.push_back(mesh->index_in_scene);
    meshes[mesh->index_in_scene] = nullptr;
    mesh->scene = nullptr;
    mesh->index_in_scene = IMeshRenderable::InvalidIndex;

    // 从更新队列中移除
    if (mesh->pending_update_index != IMeshRenderable::InvalidIndex) {
        meshes_to_update[mesh->pending_update_index] = nullptr;
        mesh->pending_update_index = IMeshRenderable::InvalidIndex;
    }
}

void RScene::gather_mesh_instances(IMeshRenderable *mesh) noexcept {
    if (!mesh->hidden && mesh->mesh) {
        Matrix4f model_matrix = mesh->transform.model_matrix();
        size_t instance_count = mesh->mesh->submeshes.size();
        GN_ASSERT(mesh->instance_indices.empty());
        mesh->instance_indices.reserve(instance_count);
        for (size_t i = 0; i < instance_count; ++i) {
            SubMesh &submesh = mesh->mesh->submeshes[i];
            if (submesh.index_count == 0) {
                continue; // 没有顶点，跳过
            }
            Ref<Material> mat = i < mesh->materials.size() ? mesh->materials[i] : nullptr;
            Handle<Instance> h_instance =
                instances.emplace(mesh->mesh, mat, submesh, mesh->per_object_uniform, mesh->transform.position,
                                  submesh.aabb.transformed(model_matrix));
            mesh->instance_indices.push_back(h_instance);
        }
    }
}

void RScene::remove_mesh_instances(IMeshRenderable *mesh) noexcept {
    for (Handle<Instance> h_instance : mesh->instance_indices) {
        instances.remove(h_instance);
    }
    mesh->instance_indices.clear();
}

void RScene::enqueue_mesh_update(IMeshRenderable *mesh) noexcept {
    if (mesh->pending_update_index != IMeshRenderable::InvalidIndex) {
        return;
    }
    mesh->pending_update_index = meshes_to_update.size();
    meshes_to_update.push_back(mesh);
}

void RScene::update_mesh(IMeshRenderable *mesh) noexcept {
    GN_ASSERT(mesh);
    GN_ASSERT(mesh->scene == this);
    GN_ASSERT(mesh->dirty_bits != std::to_underlying(IMeshRenderable::DirtyBit::None));

    enum class MeshUpdateMode {
        Transform,
        Material,
        Transform_And_Material,
        Reconstruct,
        Init,
    } mode;

    if (mesh->dirty_bits & std::to_underlying(IMeshRenderable::DirtyBit::Mesh)) {
        // 如果Mesh有变化，必须重建实例
        if (mesh->dirty_bits & std::to_underlying(IMeshRenderable::DirtyBit::Transform)) {
            // 完全初始化
            mode = MeshUpdateMode::Init;
        } else {
            // 只重建实例，不需要更新Transform
            mode = MeshUpdateMode::Reconstruct;
        }
    } else if (mesh->dirty_bits == std::to_underlying(IMeshRenderable::DirtyBit::Transform)) {
        // 如果只有Transform有变化，只需要更新Transform
        mode = MeshUpdateMode::Transform;
    } else if (mesh->dirty_bits == std::to_underlying(IMeshRenderable::DirtyBit::Material)) {
        // 如果只有Material有变化，只需要更新Material
        mode = MeshUpdateMode::Material;
    } else {
        // 如果Transform和Material都有变化，需要更新Transform和Material
        mode = MeshUpdateMode::Transform_And_Material;
    }

    switch (mode) {
    case MeshUpdateMode::Transform: {
        Matrix4f model_matrix = mesh->transform.model_matrix();
        PerObjectData per_object_data{
            .model_matrix = model_matrix.transpose(),
            .normal_matrix = Matrix4f{mesh->transform.normal_matrix().transpose()},
        };
        mesh->per_object_uniform->write(&per_object_data, BufferMapOption::WRITE_DISCARD);

        for (Handle<Instance> h_instance : mesh->instance_indices) {
            Instance &instance = instances[h_instance];
            instance.position = mesh->transform.position;
            instance.transformed_bbox = instance.submesh.aabb.transformed(model_matrix);
        }
        break;
    }
    case MeshUpdateMode::Material: {
        for (size_t i = 0; i < mesh->instance_indices.size(); ++i) {
            Ref<Material> mat = i < mesh->materials.size() ? mesh->materials[i] : nullptr;
            Instance &instance = instances[mesh->instance_indices[i]];
            instance.material = mat;
        }
        break;
    }
    case MeshUpdateMode::Transform_And_Material: {
        Matrix4f model_matrix = mesh->transform.model_matrix();
        PerObjectData per_object_data{
            .model_matrix = model_matrix.transpose(),
            .normal_matrix = Matrix4f{mesh->transform.normal_matrix().transpose()},
        };
        mesh->per_object_uniform->write(&per_object_data, BufferMapOption::WRITE_DISCARD);

        for (size_t i = 0; i < mesh->instance_indices.size(); ++i) {
            Ref<Material> mat = i < mesh->materials.size() ? mesh->materials[i] : nullptr;
            Instance &instance = instances[mesh->instance_indices[i]];
            instance.material = mat;
            instance.position = mesh->transform.position;
            instance.transformed_bbox = instance.submesh.aabb.transformed(model_matrix);
        }
        break;
    }
    case MeshUpdateMode::Reconstruct: {
        remove_mesh_instances(mesh);
        gather_mesh_instances(mesh);
        break;
    }
    case MeshUpdateMode::Init: {
        Matrix4f model_matrix = mesh->transform.model_matrix();
        PerObjectData per_object_data{
            .model_matrix = model_matrix.transpose(),
            .normal_matrix = Matrix4f{mesh->transform.normal_matrix().transpose()},
        };
        mesh->per_object_uniform->write(&per_object_data, BufferMapOption::WRITE_DISCARD);
        remove_mesh_instances(mesh);
        gather_mesh_instances(mesh);
        break;
    }
    }

    mesh->dirty_bits = std::to_underlying(IMeshRenderable::DirtyBit::None);
}

} // namespace Goonya