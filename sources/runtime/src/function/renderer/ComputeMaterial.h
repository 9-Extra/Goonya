#pragma once

#include "function/renderer/ShaderParameters.h"
#include "function/renderer/UberShader.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

class ComputeMaterial final : public ShaderParameters {
public:
    explicit ComputeMaterial(UberShader *uber_shader) : ShaderParameters(uber_shader) {
        if (!uber_shader->is_compute_shader()) {
            throw RuntimeError(std::format("\"{}\" 不是计算着色器类型的元着色器", uber_shader->get_name()));
        }
    };
    explicit ComputeMaterial(Ref<UberShader> uber_shader) : ComputeMaterial(uber_shader.get()) {}

    Ref<ComputeMaterial> clone() const noexcept;

    /**
     * @brief 绑定资源并启动
     * @param num_groups_x/y/z 工作组数量，即glDispatchCompute的实参
     * @note 内部没有加屏障，需要外部负责
     */
    void dispatch_compute(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) const;

    /**
     * @brief 绑定资源并启动，按工作量计算工作组数量
     * @param work_x/y/z 需要覆盖的总工作量（如元素个数、图像尺寸），内部按着色器声明的local_size_*向上取整为工作组数量
     * @note 向上取整意味着总会有线程被启动却无事可做，着色器必须自行判断越界并提前返回
     * @note 着色器的local_size_*可能随变体变化，这里用的是当前变体的值
     * @note 内部没有加屏障，需要外部负责
     */
    void dispatch_extent(uint32_t work_x, uint32_t work_y, uint32_t work_z) const;
};

} // namespace Goonya