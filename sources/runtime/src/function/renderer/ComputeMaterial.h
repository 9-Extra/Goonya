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
     * @note 内部没有加屏障，需要外部负责
     */
    void dispatch_compute(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) const;
};

} // namespace Goonya