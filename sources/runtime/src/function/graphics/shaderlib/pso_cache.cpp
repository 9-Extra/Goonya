#include "pso_cache.h"

namespace Goonya {
namespace Graphics {

void PSOCache::bind_pipeline_object(const PipelineStateObject& pso) const{
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

}
}