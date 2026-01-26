#version 440 core

#pragma GYA_INJECT

layout (location = 0) in vec3 position;

layout(binding = 2, std140) uniform per_pass
{
    mat4 view_perspective_matrix; //16 * 4
};

out VS_OUT
{
    vec3 cube_map_texcoords;
} vs_out;

void main()
{
    vs_out.cube_map_texcoords = position.xyz;
    vec4 pos = vec4(position, 1.0f) * view_perspective_matrix;
    gl_Position = pos.xyww;// 相当于z = w， 欺骗一下深度测试 https://www.jianshu.com/p/ad691b3ea9d5
}