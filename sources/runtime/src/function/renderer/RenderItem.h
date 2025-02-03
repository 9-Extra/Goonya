#pragma once

#include "RenderResource.h"
#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/GraphicsResource.h"

namespace Goonya {
namespace Graphics {
// 一个可渲染面片的定义
struct RenderItem {
    intrusive_ptr<Mesh> mesh;
    intrusive_ptr<Material> material;

    Matrix4 model_matrix;
    Matrix4 normal_matrix;

    RenderItem(const std::string &mesh_name, const std::string &material_name,
               const Transform &transform = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}})
        : mesh(resources.meshes.at(mesh_name)), material(resources.materials.at(material_name)),
          model_matrix(transform.model_matrix()), normal_matrix(transform.normal_matrix()) {}

    // 在遍历节点树时计算和填写
    Matrix4 world_model_matrix;
    Matrix4 world_normal_matrix;
};

} // namespace Graphics
} // namespace Goonya