#pragma once

#include "core/cgmath.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"

namespace Goonya::Graphics {

/**
 * @brief 用于记录一个网格体绘制需要的信息
 *
 * 信息包括：网格对象，材质列表，物体的UniformBuffer，其中包括变换矩阵和可能附加的逐物体数据
 * UE5中附加自定义数据的方法是在物体的UniformBuffer末尾加上一个float数组用作用户自定义数据
 * Unity使用着色器数据块对象提供任意物体数据（还能覆盖其他Uniform的数据比如材质参数）
 * Godot使用per instance uniforms在着色器中标记一些数据为per instance，之后可以在物体中设置它，在物体有多个材质时第一个材质的会覆盖后面的
 *
 * Goonya暂时先不支持
 *
 * RenderProxy由对应的Component创建并填入基础数据，然后注册到Renderer，此后RenderProxy只能由Renderer直接访问，
 * Component发生更新时通知Renderer更新RenderProxy数据
 */
struct MeshRenderProxy {
    Ref<Mesh> mesh;
    std::vector<Ref<Material>> materials;
    
    Matrix4 model_matrix;
    Matrix3 normal_matrix;
};
} // namespace Goonya::Graphics
