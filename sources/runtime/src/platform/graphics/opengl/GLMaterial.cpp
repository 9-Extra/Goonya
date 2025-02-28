#include "GLMaterial.h"

#include "core/intrusive_ptr.h"
#include "platform/graphics/graphics.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/OpenGLAPI.h"
#include "runtime/log/Log.h"

namespace Goonya {
namespace Graphics {

GLPipelineStateObject::GLPipelineStateObject(const Resource::PSODesc &desc) {
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

    shader = ((OpenGLGraphicsAPI*)graphics_api.get())->pso_cache.shader_lib.query_shader(desc.shader_desc);
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
    glUseProgram(this->shader.gl_id); // 绑定着色器
    checkError();
};

void GLMaterial::update_parameters() const noexcept {
    if (!is_dirty) {
        return;
    }
    {
        const auto &layout = get_shader().per_material.layout;
        std::unique_ptr<uint8_t> memory{new uint8_t[layout.size]};
        auto w = Meta::DynamicStructWriter(layout, memory.get());
        for (const auto &[name, value] : parameters) {
            w.set_field(name, value);
        }
        // 直接创建新的
        per_material =
            intrusive_ptr<UniformBuffer>(new GLUniformBuffer{std::span(memory.get(), layout.size), BufferType::STATIC});
    }
    is_dirty = false;
}

void GLMaterial::bind() const {
    checkError();
    pso->bind(); // 绑定此材质关联的着色器
    this->update_parameters();
    // 绑定材质的uniform buffer
    ((GLUniformBuffer*)per_material.get())->bind_uniform(get_shader().per_material.binding);
    // 绑定所有纹理
    for (const auto&  [id, t] : textures) {
        t->bind(id);
    }
    checkError();
}

} // namespace Graphics
} // namespace Goonya