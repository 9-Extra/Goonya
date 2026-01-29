#name depth
#texture skybox_specular_texture="buildin:missing_texture"
#setting depth_test="less_equal"
#setting cull_mode="off"

#param color_permutation=vec3(1, 1, 1)

#section common
#include "shaders/common.glsl"

#section vertex

void vert()
{
    gl_Position = vec4(position.xyz, 1.0f) * model_matrix * view_perspective_matrix;
}

#section fragment

void frag() {
    // noop
}