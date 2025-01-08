#include "pso_cache.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

void PSOCache::bind_pipeline_object(const PipelineStateObject& pso) const noexcept{
    const PipelineStateContainer& container = containers[pso.id];
    if (container.enable_cilp){
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glCullFace(container.cull_face_mode);
    glFrontFace(container.front_face_clockwise);

    if (container.enable_depth_test){
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthFunc(container.depth_func);

    glUseProgram(container.shaderprogram_id); // 绑定着色器
}

PipelineStateContainer PSOCache::load_pso(const Resource::PSODesc &desc) {
    PipelineStateContainer container;
    container.enable_cilp = desc.enable_cilp;
    if (desc.cull_face_mode == "front") {
        container.cull_face_mode = GL_FRONT;
    } else if (desc.cull_face_mode == "back") {
        container.cull_face_mode = GL_BACK;
    } else if (desc.cull_face_mode == "front") {
        container.cull_face_mode = GL_FRONT_AND_BACK;
    } else {
        throw RuntimeError(std::format("不支持的面裁剪模式：\"{}\"", desc.cull_face_mode));
    }

    if (desc.front_face_clockwise == "clockwise") {
        container.front_face_clockwise = GL_CW;
    } else if (desc.front_face_clockwise == "counterclockwise") {
        container.front_face_clockwise = GL_CCW;
    } else {
        throw RuntimeError(std::format("不支持的模式：\"{}\"", desc.front_face_clockwise));
    }

    container.enable_depth_test = desc.enable_depth_test;

    if (desc.depth_func == "never") {
        container.depth_func = GL_NEVER;
    } else if (desc.depth_func == "less") {
        container.depth_func = GL_LESS;
    } else if (desc.depth_func == "less_equal") {
        container.depth_func = GL_LEQUAL;
    } else if (desc.depth_func == "greater") {
        container.depth_func = GL_GREATER;
    } else if (desc.depth_func == "greater_equal") {
        container.depth_func = GL_GEQUAL;
    } else if (desc.depth_func == "always") {
        container.depth_func = GL_ALWAYS;
    } else {
       throw RuntimeError(std::format("不支持的深度测试方法：\"{}\"", desc.depth_func));
    }

    container.shaderprogram_id = shader_lib.query_shader(desc.shader_desc).gl_id;

    return container;
};
} // namespace Graphics
}