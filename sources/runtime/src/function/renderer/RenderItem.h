#pragma once

#include <glad/glad.h>

#include <cstdint>
#include "core/cgmath.h"
#include "RenderResource.h"


namespace Goonya {
namespace Graphics {
// 一个可渲染面片的定义
struct RenderItem {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology;
    Matrix4 model_matrix;
    Matrix4 normal_matrix;

    RenderItem(const std::string &mesh_name, const std::string &material_name,
                   const uint32_t topology = GL_TRIANGLES,
                   const Transform &transform = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}})
        : mesh_id(resources.meshes.find(mesh_name)), material_id(resources.materials.find(material_name)),
          topology(topology), model_matrix(transform.transform_matrix()), normal_matrix(transform.normal_matrix()) {}
    
    //在遍历节点树时计算和填写
    Matrix4 root_transform;
    Matrix4 root_normal_matrix;
};

}
}