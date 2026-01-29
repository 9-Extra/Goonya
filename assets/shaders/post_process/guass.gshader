#name guass_blur
#setting depth_test="off"
#setting cull_mode="off"

#texture source="buildin:missing_texture"

#param blur_size=f32(2.0f)

#local_variant HORIZONTAL VERTICAL

#section common

#include "post_process.glsl"

uniform sampler2D source;

VS_OUT_PS_IN VS_OUT
{
    vec2 uv[5];
} vs_out;

#section vertex

void vert()
{
    vec2 origin_uv = uv[index[gl_VertexID]];
    vec2 texel_size = vec2(1.0f) * blur_size / textureSize(source, 0);
#if defined HORIZONTAL
    for(int i = 0;i < 5;i++){
        vs_out.uv[i] = vec2(origin_uv.x + texel_size.x * (i - 2), origin_uv.y);
    }
#elif defined VERTICAL
    for(int i = 0;i < 5;i++){
        vs_out.uv[i] = vec2(origin_uv.x, origin_uv.y + texel_size.y * (i - 2));
    }
#endif
    gl_Position = vec4(position[index[gl_VertexID]], 1.0f);
}

#section fragment

layout(location = 0) out vec4 out_color;

void frag()
{
    const float weight[5] = float[5](0.0545, 0.2442, 0.4028, 0.2442, 0.0545);
    vec3 sum = vec3(0);
    for(int i = 0;i < 5;i++){
        vec3 pixel = textureLod(source, vs_out.uv[i], 0).rgb;
        sum += pixel * weight[i];
    }
    out_color = vec4(sum, 1.0f);
}