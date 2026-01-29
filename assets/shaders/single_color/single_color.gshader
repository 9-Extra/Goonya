#name skybox

#param color_permutation=vec3(0, 1, 0)
#texture base_color="buildin:white"

#local_variant _ USE_TEXTURE

#section common
#include "shaders/common.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec2 uv;
} vs_out;

#section vertex

void vert()
{
    vs_out.uv = uv;
    gl_Position = vec4(position.xyz, 1.0f) * model_matrix * view_perspective_matrix;
}
#section fragment

uniform sampler2D base_color;

layout(location = 0) out vec4 out_color;

void frag()
{
#if defined(USE_TEXTURE)
    out_color = vec4(texture(base_color, vs_out.uv).rgb * color_permutation, 1.0f);
#else
    out_color = vec4(color_permutation, 1.0f);
#endif
}