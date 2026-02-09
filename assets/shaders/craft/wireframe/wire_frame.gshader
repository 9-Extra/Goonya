#name skybox
#texture skybox_specular_texture="buildin:missing_texture"
#setting depth_test="less_equal"
#setting cull_mode="off"

#section common
#include "shaders/common.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec3 model_position;
} vs_out;

#section vertex

void vert()
{
    const float line_width = 2.5;
    vec4 line_start = vec4(position, 1.0f) * model_matrix * view_perspective_matrix; 
    vec4 line_end = vec4((position + normal), 1.0f) * model_matrix * view_perspective_matrix;

    vec3 ndc_line_start = line_start.xyz / line_start.w;
    vec3 ndc_line_end = line_end.xyz / line_end.w; 

    // 在屏幕空间中线的方向
    vec2 line_direction_screen = normalize((ndc_line_end.xy - ndc_line_start.xy) * screen_size);
    // 取线方向的垂直方向，放大线宽倍数，最后除以屏幕大小，保证在任意屏幕大小下宽度一致
    vec2 ndc_line_offset = vec2(-line_direction_screen.y, line_direction_screen.x) * line_width / screen_size;

    // 尽管这里实际上已经求出ndc坐标了，但是为了不破坏opengl的透视矫正，需要将其还原为齐次坐标
    float w = line_start.w; 
    if (gl_VertexID % 2 == 0){
        vec3 ndc_postion = ndc_line_start + vec3(ndc_line_offset, 0);
        gl_Position = vec4(ndc_postion * w, w);
    } else {
        vec3 ndc_postion = ndc_line_start - vec3(ndc_line_offset, 0);
        gl_Position = vec4(ndc_postion * w, w);
    }
}

#section fragment

layout(location = 0) out vec4 out_color;

void frag() {
    out_color = vec4(0, 0, 0, 1.0f);
}