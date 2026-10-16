#name brdf_lut
#category compute

#image output_image access="w" format="rg16f"

#section compute

#include "shaders/common.glsl"

#define SAMPLE_COUNT 512

layout(rg16f, binding = 2) restrict writeonly uniform image2D output_image; // LUT

layout(local_size_x = 16, local_size_y = 16) in;

/*
计算用于Split-Sum近似的BRDF预积分贴图，这张贴图与场景和物体属性都无关，因此整个引擎共用这一张
因为旋转对称和各项同性，所有计算在 n 恒等于 (0,0,1) 的坐标系里完成（类似切空间，但T和B不重要），整个LUT只有一个几何参数v * n

目标是预计算函数的期望 DFG(v, n, r) = E(F(v * h) * G(v, l, n) / ((n * v) * (n * h))) ，其中h = normalize(v + l)，
使用蒙特卡洛积分，其中 l 来自 GGX重要性采样（它不是参数，是积分变量）。
采样函数为，设u_1，u_2服从[0, 1]均匀分布，cos(\theta) = sqrt((1 - u_1) / (1 + (alpha ^ 2 - 1) * u_1)) \phi = 2 * PI * u_2
，切空间中h = (sin(\theta)cos(\phi),sin(\theta)sin(\phi), cos(\theta))

贴图LUT将其简化为 DFG(v, n, r) = F_0  * LUT(v * n, r).r + LUT(v * n, r).g  
LUT(v * n, r).r = G(v, l, n) * (v * h) / ((n * v) * (n * h)) * (1 - (1 - v * h)^5)
LUT(v * n, r).g = G(v, l, n) * (v * h) / ((n * v) * (n * h)) * (1 - v * h)^5

中间函数有：
F(v * h) = F_0 + (1 - F_0) (1 - v * h)^5 （Schlick 近似）
G(v, l, n) 见 G_SchlicksmithGGX_IBL

本贴图有2维度2通道，2个维度
*/

// 取(u_1, u_2)
vec2 hammersley2d(uint i, uint N) {
    // 将i的二进制位反转，映射到[0,1)
    float phi = float(bitfieldReverse(i)) * 2.3283064365386963e-10f; // 1/(2^32)
    return fract(vec2(float(i)/float(N), phi));
}

vec3 importance_sample_ggx_h(vec2 u, float r) {
    float alpha = r * r;
    // 这里存在一个数值精度问题，r 接近 0 且 u.x = 1 时会出现 0 / 0 导致精度误差甚至NaN
    // 另外GPU计算中默认使用近似的精度，不保证 x / x == 1，结果可能大于1
    // 原始形式为 float cos_theta_sqr = (1 - u.x) / (1 + (alpha * alpha - 1) * u.x);
    float cos_theta_sqr = (1 - u.x) / max(0.0000001, (1 - u.x + alpha * alpha * u.x)); // 调整顺序降低误差，保证不被0除
    cos_theta_sqr = min(cos_theta_sqr, 1); // 防止精度误差cos_theta_sqr会超过1导致的NaN
    float phi = 2 * PI * u.y;
    
    float cos_theta = sqrt(cos_theta_sqr);
    float sin_theta = sqrt(1 - cos_theta_sqr);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    vec3 h = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
    return h;
}

// UE4的GGX-Smith Correlated Joint 近似方案
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
// from https://zhuanlan.zhihu.com/p/81708753
float G_SmithJointApprox(float NoV, float NoL, float roughness)
{
    float a = roughness * roughness;
    float Vis_SmithV = NoL * ( NoV * ( 1 - a ) + a );
    float Vis_SmithL = NoV * ( NoL * ( 1 - a ) + a );
    return 0.5 / (Vis_SmithV + Vis_SmithL) * (4 * NoL * NoV);
}

float G_SchlicksmithGGX_IBL(float dotNL, float dotNV, float roughness)
{
    float alpha  = roughness * roughness;
    float k  = alpha / 2;
    float GL = dotNL / (dotNL * (1.0 - k) + k); // Schlick-GGX 计算遮蔽或者阴影，RTR4上没找到？
    float GV = dotNV / (dotNV * (1.0 - k) + k); 
    return GL * GV; // 使用Smith的方法乘起来计算两个效应的叠加，RTR4上有这个
}


vec2 compute_lut(int index, float dotNV, float roughness) {
    vec3 n = vec3(0, 0, 1);
    vec3 h = importance_sample_ggx_h(hammersley2d(index, SAMPLE_COUNT), roughness);
    vec3 v = vec3(sqrt(1 - dotNV * dotNV), 0, dotNV); // 重建v，方向不重要
    float dotNL = 2 * dot(v, h) * dot(n, h) - dot(n, v); // 直接算出n ⋅ l，不需要重建L
    if (dotNL <= 0) return vec2(0, 0);

    float dotNH = dot(n, h);
    float dotVH = dot(v, h);
    
    float G = G_SchlicksmithGGX_IBL(dotNL, dotNV, roughness);

    float G_vis = G * dot(v, h) / (dotNV * dotNH);
    float F = pow(1 - dotVH, 5);
    float A = G_vis * (1 - F);
    float B = G_vis * F;

    return vec2(A, B);
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    if (x >= imageSize(output_image).x || y >= imageSize(output_image).y) {
        return;
    }

    float dotNV = (x + 0.5) / imageSize(output_image).x;
    float roughness = (y + 0.5) / imageSize(output_image).y;

    vec2 result = vec2(0);
    for(int i = 0; i < SAMPLE_COUNT;i++){
        vec2 value = compute_lut(i, dotNV, roughness);
        result += value;
    }
    result /= SAMPLE_COUNT;
    imageStore(output_image, ivec2(x, y), vec4(result, 0, 0));
}
