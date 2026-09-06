#pragma once

#include "core/RefCount.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"
#include "function/renderer/Material.h"
#include "function/renderer/Mesh.h"
#include "function/renderer/RScene.h"
#include "function/renderer/SkinningMesh.h"
#include "function/world/Component.h"
#include "function/world/GObject.h"
#include "function/world/World.h"
#include "runtime/GAssert.h"

#include <concepts>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace Goonya {
// 渲染mesh的组件，可以渲染出物体
class CpntSkinMeshRender : public Component, public TickFunction {
private:
    RScene *scene = nullptr;
    RenderableRef renderable;

    Ref<SkinningMesh> mesh;
    std::vector<Ref<Material>> materials;
    std::string root_path; // 定义的骨骼根路径，在gltf中就是场景的根，空字符串视为未定义，指向自己需使用"./"
    std::vector<std::string> joint_paths; // 绑定的关节路径
    std::vector<Matrix4f> joint_ibms;     // 绑定的关节的逆绑定矩阵

    std::vector<SkinMeshVertex> caculated_mesh_data;
    std::vector<std::weak_ptr<GObject>> binding_joints; // 绑定的关节引用

public:
    std::unique_ptr<Component> clone() const override;

    void on_register() override;

    Ref<Mesh> get_mesh() const noexcept { return mesh; }
    const std::vector<Ref<Material>> &get_materials() const noexcept { return materials; }

    void set_base_mesh(const Ref<Mesh> &mesh);
    void set_bone_root(std::string_view path);
    template <typename T1, typename T2>
        requires std::constructible_from<T1, std::vector<std::string>> &&
                 std::constructible_from<T2, std::vector<Matrix4f>>
    void set_joints(T1 &&joint_paths, T2 &&joint_ibms) {
        GN_ASSERT_MSG(joint_paths.size() == joint_ibms.size(), "关节数和拟绑定矩阵数量必须一致");
        this->joint_paths = std::forward<T1>(joint_paths);
        this->joint_ibms = std::forward<T2>(joint_ibms);
    }
    void set_material(uint32_t slot, const Ref<Material> &material) noexcept;
    void set_materials(std::span<Ref<Material>> materials) noexcept;

    void on_unregister() override;
    void on_update(ComponentUpdateFlag flag) override;
    void tick() override { cpu_mesh_update(); }

private:
    // 更新绑定的关节节点引用
    void update_joint_cache();
    void cpu_mesh_update();
};

} // namespace Goonya
