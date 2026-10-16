#name texture_to_screen
#setting depth_test="off"
#setting cull_mode="off"

#local_variant _ DO_GAMMA

#param xywh = vec4(0.25, 0.25, 0.5, 0.5)
#param mipmap_level = f32(0)
#param gamma = f32(1)

#texture source="buildin:white"

#section common

#include "post_process.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec2 uv;
} vs_out;

#section vertex

void vert()
{
    vs_out.uv = uv[index[gl_VertexID]];
    vec2 pos_xy = position[index[gl_VertexID]].xy;
    pos_xy = (pos_xy + 1) * xywh.zw - vec2(1) + xywh.xy * 2;
    gl_Position = vec4(pos_xy, -1.0f, 1.0f);
}

#section fragment

layout(location = 0) out vec4 out_color;

layout(binding = 8) uniform sampler2D source;

vec3 color_encode(vec3 color_in){
    return pow(color_in, vec3(1 / gamma));
}
void frag()
{
    vec3 pixel = textureLod(source, vs_out.uv, mipmap_level).rgb;
#if defined(DO_GAMMA)    
    pixel = color_encode(pixel);
#endif
    out_color = vec4(pixel, 1.0f);
}