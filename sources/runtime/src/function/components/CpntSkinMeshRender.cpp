#include "CpntSkinMeshRender.h"
#include "core/RefCount.h"
#include "core/cgmath/matrix.h"
#include "core/log/Log.h"
#include "function/renderer/Renderer.h"
#include "runtime/GAssert.h"

namespace Goonya {

std::unique_ptr<Component> CpntSkinMeshRender::clone() const {
    auto new_comp = std::make_unique<CpntSkinMeshRender>();
    new_comp->mesh = mesh;
    new_comp->materials = materials;
    new_comp->root_path = root_path;
    new_comp->joint_paths = joint_paths;
    new_comp->joint_ibms = joint_ibms;
    return new_comp;
}
void CpntSkinMeshRender::on_register() {
    GN_ASSERT(get_owner() != nullptr);
    GObject &owner = *get_owner();
    scene = owner.get_world()->get_scene();
    renderable = RenderableRef{*scene};
    // 蒙皮网格不响应对应物体的变换：对骨骼根节点进行的变换已经反应到网格上，防止二次变换
    // renderable.set_transform(owner.get_world_model_matrix());
    if (mesh) {
        renderable.set_skinning_mesh(mesh);
    }
    if (!materials.empty()) {
        renderable.set_materials(materials);
    }
    update_joint_cache();
    register_ticker(owner.get_world());
}
void CpntSkinMeshRender::set_base_mesh(const Ref<Mesh> &mesh) {
    this->mesh = Ref(new SkinningMesh{*mesh});
    if (renderable.is_valid()) {
        renderable.set_mesh(this->mesh);
    }
}
void CpntSkinMeshRender::set_bone_root(std::string_view path) {
    if (path == root_path) {
        return;
    }
    root_path = path;
    if (scene) { // registered
        update_joint_cache();
    }
}
void CpntSkinMeshRender::set_material(uint32_t slot, const Ref<Material> &material) noexcept {
    if (materials.size() <= slot) {
        materials.resize(slot + 1, nullptr);
    }
    materials[slot] = material;
    if (renderable.is_valid()) {
        renderable.set_materials(materials);
    }
}
void CpntSkinMeshRender::set_materials(std::span<Ref<Material>> materials) noexcept {
    this->materials.assign_range(materials);
    if (renderable.is_valid()) {
        renderable.set_materials(this->materials);
    }
}
void CpntSkinMeshRender::on_unregister() {
    renderable = {};
    scene = nullptr;
    binding_joints.clear();
    unregister_ticker();
}
void CpntSkinMeshRender::on_update(ComponentUpdateFlag flag) {
    GN_ASSERT(renderable.is_valid());

    // 蒙皮网格不响应对应物体的变换：对骨骼根节点进行的变换已经反应到网格上，防止二次变换
    /*
    if (contain(flag, ComponentUpdateFlag::TRANSFORM)) {
        renderable.set_transform(get_owner()->get_world_model_matrix());
    }
    */
}
void CpntSkinMeshRender::update_joint_cache() {
    if (root_path.empty()) {
        binding_joints.clear();
        return;
    }
    std::shared_ptr<GObject> bone_root = get_owner()->get_child_by_path(root_path);
    if (!bone_root) {
        LOG_WARN("无效节点路径{}", root_path);
    }
    binding_joints.clear();
    binding_joints.reserve(joint_paths.size());
    for (std::string_view p : joint_paths) {
        std::shared_ptr<GObject> joint = bone_root->get_child_by_path(p);
        if (!joint) {
            LOG_WARN("关节查找失败，从节点\"{}\"查找\"{}\"", bone_root->get_name(), p);
        }
        binding_joints.emplace_back(std::move(joint));
    }
}
void CpntSkinMeshRender::update_pose_matrix() {
    if (!scene || !mesh || binding_joints.empty()) {
        return;
    }
    auto pose_matrix = mesh->map_pose_matrix(binding_joints.size());
    // 姿态矩阵和它的逆的转置
    for (auto &&[i, joint] : std::views::enumerate(binding_joints)) {
        auto j = joint.lock();
        Matrix4f world_matrix = j ? j->get_world_model_matrix() : Matrix4f::identity();
        // 骨骼空间下蒙皮位置 * 绑定矩阵 == 原始网格体中的顶点位置
        // 骨骼空间下蒙皮位置 * 当前的节点世界变换 == 目标位置
        // 目标位置 = 原始网格体中的顶点位置 * 绑定矩阵^{-1} * 当前的节点世界变换
        Matrix4f p = joint_ibms[i] * world_matrix;
        Matrix4f n = p.inverse().value_or(Matrix4f::identity()).transpose();

        if (renderer.skinning_on_gpu) {
            // 上传gpu需要转置
            pose_matrix[i] = {p.transpose(), n.transpose()};
        } else {
            pose_matrix[i] = {p, n};
        }
    }
    mesh->unmap_pose_matrix();
}
} // namespace Goonya