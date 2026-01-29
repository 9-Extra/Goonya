#name terrain
#setting depth_test="less_equal"
#setting cull_mode="back"

#texture basecolor_texture="buildin:missing_texture"

#section common
#include "shaders/common.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec3 world_position;
    vec2 tex_coords;
    flat uint object_id;
} vs_out;

#section vertex

void vert()
{
    uint object_id = gl_VertexID / 4;
    vs_out.world_position = position;
    vs_out.tex_coords = uv;
    vs_out.object_id = object_id;

    gl_Position = vec4(position, 1) * view_perspective_matrix;
}

#section fragment

struct PerSurface{
    uint basecolor_id;
    vec3 tint_color;
    vec3 normal;
};

layout(binding = 4, std430) buffer per_surface{
    PerSurface surfaces[]; // 使用SSBO实现动态大小
};

layout(binding = 0) uniform sampler2DArray basecolor_texture;

layout(location = 0) out vec4 out_color;

const vec3 sh_coeffs[9] = vec3[9](
    vec3(0.89f, 0.67f, 0.34f) * 1.5f,
    vec3(0.5f, 0.5f, 0.5f), // y
    vec3(0.1f, 0.1f, 0.1f), // z
    vec3(0.1f, 0.1f, 0.1f), // x
    vec3(0.1f, 0.1f, 0.15f),
    vec3(0.1f, 0.0f, 0.0f),
    vec3(0.05f, 0.05f, 0.1f),
    vec3(0.0f, 0.0f, 0.0f),
    vec3(0.08f, 0.08f, 0.12f)
);

// 三阶球谐基函数
vec3 spherical_harmonics3(vec3 direction)
{
    // 确保方向归一化
    vec3 n = normalize(direction);
    float x = n.x;
    float y = n.y;
    float z = n.z;
    
    // 三阶球谐基函数 (l=0 到 l=2)
    float basis[9];
    
    // l = 0
    basis[0] = 0.2820947918; // Y00 = 1/(2√π)
    
    // l = 1
    basis[1] = 0.4886025119 * y; // Y1-1
    basis[2] = 0.4886025119 * z; // Y1+0
    basis[3] = 0.4886025119 * x; // Y1+1
    
    // l = 2
    basis[4] = 1.0925484306 * x * y; // Y2-2
    basis[5] = 1.0925484306 * y * z; // Y2-1
    basis[6] = 0.3153915652 * (3.0 * z * z - 1.0); // Y2+0
    basis[7] = 1.0925484306 * x * z; // Y2+1
    basis[8] = 0.5462742153 * (x * x - y * y); // Y2+2
    
    // 计算球谐光照
    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; i++)
    {
        result += sh_coeffs[i].rgb * basis[i];
    }
    
    return max(result, vec3(0.0));
}

void frag()
{
    PerSurface surface = surfaces[vs_out.object_id];
    vec4 base_color = texture(basecolor_texture, vec3(vs_out.tex_coords, surface.basecolor_id));
    float alpha = base_color.a;
    if (alpha < 0.1)
        discard; // 启用透明度测试，渲染个草方块都得要这玩意儿

    vec3 color = base_color.rgb * surface.tint_color;
    vec3 light = spherical_harmonics3(surface.normal);

    vec3 linear_color = color.rgb * light;

    out_color = vec4(linear_color, 1.0f);
    // out_color = vec4(abs(surface.normal), 1);
}