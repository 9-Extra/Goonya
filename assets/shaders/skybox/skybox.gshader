#name skybox
#texture skybox_specular_texture="buildin:missing_texture"
#setting depth_test="less_equal"
#setting cull_mode="off"

#param color_permutation=vec3(1, 1, 1)

#section common
#include "shaders/common.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec3 cube_map_texcoords;
} vs_out;

#section vertex

layout(binding = 2, std140) uniform per_pass
{
    mat4 skybox_perspective_matrix; //16 * 4
};

void vert() {
    vs_out.cube_map_texcoords = position.xyz;
    vec4 pos = vec4(position, 1.0f) * skybox_perspective_matrix;
    gl_Position = pos.xyww;// 相当于z = w， 欺骗一下深度测试 https://www.jianshu.com/p/ad691b3ea9d5
}

#section fragment

uniform samplerCube skybox_specular_texture;

layout(location = 0) out vec4 out_color;

void frag() {
    vec3 result_color = texture(skybox_specular_texture, vs_out.cube_map_texcoords).xyz * color_permutation;
    out_color = vec4(result_color, 1.0f);
}