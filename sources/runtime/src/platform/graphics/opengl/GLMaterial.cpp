#include "GLMaterial.h"

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "function/renderer/RenderResource.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "runtime/GoonyaException.h"
#include <cassert>

namespace Goonya {
namespace Graphics {

GLPipelineStateObject::GLPipelineStateObject(const PSODesc &desc) {
    enable_cilp = desc.enable_cilp;
    if (desc.cull_face_mode == "front") {
        cull_face_mode = GL_FRONT;
    } else if (desc.cull_face_mode == "back") {
        cull_face_mode = GL_BACK;
    } else if (desc.cull_face_mode == "front_back") {
        cull_face_mode = GL_FRONT_AND_BACK;
    } else {
        throw RuntimeError(std::format("不支持的面裁剪模式：\"{}\"", desc.cull_face_mode));
    }

    if (desc.front_face_clockwise == "clockwise") {
        front_face_clockwise = GL_CW;
    } else if (desc.front_face_clockwise == "counterclockwise") {
        front_face_clockwise = GL_CCW;
    } else {
        throw RuntimeError(std::format("不支持的模式：\"{}\"", desc.front_face_clockwise));
    }

    enable_depth_test = desc.enable_depth_test;

    if (desc.depth_func == "never") {
        depth_func = GL_NEVER;
    } else if (desc.depth_func == "less") {
        depth_func = GL_LESS;
    } else if (desc.depth_func == "less_equal") {
        depth_func = GL_LEQUAL;
    } else if (desc.depth_func == "greater") {
        depth_func = GL_GREATER;
    } else if (desc.depth_func == "greater_equal") {
        depth_func = GL_GEQUAL;
    } else if (desc.depth_func == "always") {
        depth_func = GL_ALWAYS;
    } else {
        throw RuntimeError(std::format("不支持的深度测试方法：\"{}\"", desc.depth_func));
    }

    shader = static_intrusive_ptr_cast<GLShader>(resources.shader_lib->query_shader(desc.shader_desc));
    assert(shader);
}

void GLPipelineStateObject::bind() const {
    if (this->enable_cilp) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    glCullFace(this->cull_face_mode);
    glFrontFace(this->front_face_clockwise);
    if (this->enable_depth_test) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthFunc(this->depth_func);
    this->shader->bind(); // 绑定着色器
    opengl_debug_check_error();
};

void GLMaterial::update() {
    reset_pso();
    update_parameter();
}

void GLMaterial::bind() {
    opengl_debug_check_error();
    this->update();

    pso->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    per_material->bind_uniform(get_shader()->get_per_material_layout().binding);
    // 绑定所有纹理
    for (const auto&  [unit, t] : textures) {
        t->bind(unit);
    }
    opengl_debug_check_error();
}

void GLMaterial::reset_pso() {
    if (!is_pso_dirty)
        return;
    
    pso = resources.shader_lib->query_pso(pso_desc);
    const auto &layout = get_shader()->get_per_material_layout().layout;
    if (!per_material || per_material->get_size() != layout.size){
        // 材质参数Buffer大小可能发生变化，必要生成新的
        per_material = intrusive_ptr<GLBuffer>(new GLBuffer{layout.size, BufferType::DYNAMIC});
    }

    // 更新纹理
    for (const auto& [sampler_name, unit]: get_shader()->get_texture_units()){
        if (!texture_info.contains(sampler_name)){
            throw RuntimeError(std::format("材质需要的纹理{}未设置", sampler_name));
        }
        textures[unit] = texture_info.at(sampler_name);
    }

    is_parameters_dirty = true;

    is_pso_dirty = false;
}
void GLMaterial::update_parameter() {
    if (!is_parameters_dirty)
        return;
    const auto &layout = get_shader()->get_per_material_layout().layout;
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