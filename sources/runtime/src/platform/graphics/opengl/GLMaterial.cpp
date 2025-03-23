#include "GLMaterial.h"

#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/opengl/GLBasic.h"
#include <cassert>
#include <utility>

namespace Goonya {
namespace Graphics {

void GLMaterial::set_pipeline_state() const noexcept {
    // 深度测试
    bool enable_depth_test = true;
    GLenum depth_func = 0;

    switch (this->depth_test) {
    case DepthTestMode::LESS: {
        depth_func = GL_LESS;
        break;
    }
    case DepthTestMode::LESS_EQUAL: {
        depth_func = GL_LEQUAL;
        break;
    }
    case DepthTestMode::GREATER: {
        depth_func = GL_GREATER;
        break;
    }
    case DepthTestMode::GREATER_EQUAL: {
        depth_func = GL_GEQUAL;
        break;
    }
    case DepthTestMode::NEVER: {
        depth_func = GL_NEVER;
        break;
    }
    case DepthTestMode::ALWAYS: {
        depth_func = GL_ALWAYS;
        break;
    }
    case DepthTestMode::DISABLE: {
        enable_depth_test = false;
        break;
    }
    default:
        std::unreachable();
    }

    if (enable_depth_test) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depth_func);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    bool enable_cull_face = true;
    GLenum gl_cull_mode = 0;

    switch (this->cull_mode) {

    case CullFaceMode::BACK: {
        gl_cull_mode = GL_BACK;
        break;
    }
    case CullFaceMode::FRONT:{
        gl_cull_mode = GL_FRONT;
        break;
    }
    case CullFaceMode::FRONT_AND_BACK:{
        gl_cull_mode = GL_FRONT_AND_BACK;
        break;
    }
    case CullFaceMode::DISABLE:{
        enable_cull_face = false;
        break;
    default:
        std::unreachable();
    }
    }

    if (enable_cull_face){
        glEnable(GL_CULL_FACE);
        glCullFace(gl_cull_mode);
    } else {
        glDisable(GL_CULL_FACE);
    }

    opengl_debug_check_error();
};

void GLMaterial::update() {
    update_shader_variant();
    update_parameter();
}

void GLMaterial::bind() {
    opengl_debug_check_error();
    this->update();

    set_pipeline_state();
    shader->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    per_material->bind_uniform(uber_shader->per_material_block().binding);
    // 绑定所有纹理
    for (const auto &[unit, t] : textures) {
        t->bind(unit);
    }
    opengl_debug_check_error();
}

void GLMaterial::update_shader_variant() {
    VariantCodeSet code{
        .global_code = uber_shader->get_global_key_code(),
        .loacl_code = local_variant_code
    };
    if (current_variant_code != code){
        shader = uber_shader->query_variant(code);
        current_variant_code = code;
    }
}

void GLMaterial::update_parameter() {
    if (!is_parameters_dirty)
        return;
    const auto &layout = uber_shader->per_frame_block().layout;
    {
        auto w = DynamicBufferWriter(per_material, layout);
        for (const auto &[name, value] : parameters) {
            w.set_field(name, value);
        }
    }

    is_parameters_dirty = false;
}
} // namespace Graphics
} // namespace Goonya