#pragma once

#include "core/RefCount.h"
#include "core/log/Log.h"

#include <cassert>
#include <cstdint>
#include <glad/glad.h>
#include <string>

namespace Goonya {

/**
 * @brief 编译完成的计算着色器
 */
class GLComputeShader final : public RefCount {
private:
    GLuint id = 0;

public:
    explicit GLComputeShader(const std::string &src) {
        assert(false); // todo，用的时候再完善
    }
    ~GLComputeShader() { glDeleteProgram(id); }

    /**
     * @brief 发起计算着色器调用
     * 参数为工作组数量，分三个维度
     */
    void dispatch_compute(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) const {
        glUseProgram(id);
        glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    }

    void set_texture_binding(const std::string &name, uint32_t unit) const noexcept {
        GLint location = glGetUniformLocation(id, name.c_str());
        if (location != -1) {
            glProgramUniform1i(id, location, unit);
        } else {
            LOG_WARN("着色器中未找到纹理{}", name);
        }
    }
    void set_texture_binding(uint32_t location, uint32_t unit) const noexcept {
        glProgramUniform1i(id, location, unit);
    }
    GLuint get_id() const { return id; }
};

}; // namespace Goonya