#version 440 core

#pragma GYA_INJECT

#define POINTLIGNT_MAX 8

struct PointLight{
    vec3 position;
    vec3 intensity;
};

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix; //16 * 4
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    float time;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

layout(binding = 2) uniform per_material
{
    float metallic_factor;
    float roughness_factor;
};

in VS_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

layout(binding = 0) uniform sampler2D basecolor_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 2) uniform sampler2D metallic_roughness_texture;
layout(binding = 5) uniform samplerCube skybox_specular_texture;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

// ===========================================================================

#define PI 3.1415926

struct PixelArribute{
    vec3 albedo;
    vec3 emission; // 自发光颜色
    float roughness;
    float metallic;
    vec3 F0;
}; 

// 生成 [0,1) 的Hammersley点（索引 i，总样本数 N），爱来自DeepSeek
vec2 hammersley2d(uint i, uint N) {
    // 将i的二进制位反转，映射到[0,1)
    float phi = float(bitfieldReverse(i)) * 2.3283064365386963e-10f; // 1/(2^32)
    return vec2(float(i)/float(N), phi);
}

// RTR4 9.41 P340，GGX分布
// 这里dotNH是法线方向和指定方向的夹角cos，返回指定方向上的微表面密度，即costheta
float D_GGX(float dotNH, float roughness)
{
    // 为了让粗糙度更线性，暴露给用户的roughness是GGX公式中alpha的开方
    float alpha  = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom  = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
// https://learnopengl-cn.github.io/07%20PBR/01%20Theory/
// 计算了微表面上的遮蔽和阴影
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r  = (roughness + 1.0);
    float k  = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k); // Schlick-GGX 计算遮蔽或者阴影，RTR4上没找到？
    float GV = dotNV / (dotNV * (1.0 - k) + k); 
    return GL * GV; // 使用Smith的方法乘起来计算两个效应的叠加，RTR4上有这个
}

// 这是一种拟合方式
vec3 F_Schlick(float cosTheta, vec3 F0) 
{ 
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5); 
}

// Specular and diffuse BRDF composition --------------------------------------------
vec3 BRDF(vec3 L, vec3 V, vec3 N, PixelArribute pixel_attribute)
{
    // Precalculate vectors and dot products
    vec3  H     = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    float roughness = max(0.05, pixel_attribute.roughness);
    // D 微表面法线分布
    float D = D_GGX(dotNH, roughness);
    // G = Geometric shadowing term (Microfacets shadowing)
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
    // F = Fresnel factor (Reflectance depending on angle of incidence)
    vec3 F = F_Schlick(dotNV, pixel_attribute.F0);

    vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001); // 高光BRDF
    
    // 计算漫反射
    vec3 diff = pixel_attribute.albedo / PI; // lambert diffuse
    // 漫反射是由折射光在的次表面散射产生的，并且金属没有次表面散射，由此计算系数kD
    vec3 kD = (vec3(1) - F) * (1.0 - pixel_attribute.metallic);
    return kD * diff + spec;
}

// 爱来自虚幻4，https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// 输入二维(0, 1)随机数Xi（可用低差异序列），粗糙度Roughness和法线方向N, 采样一个方向向量
vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
    vec3 H; // 位于切线空间的方向，法线方向为+z
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    // 使用法线方向N随便构造一组基以转换到世界坐标系，即随便找两个和N垂直的方向向量
    // 理论上随便找个向量和N叉乘就能找到与N垂直的向量，为了数值精度这个向量不能和N太相近
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize(cross(UpVector, N)); // 找一个和N垂直的方向
    vec3 TangentY = cross(N, TangentX);
    // Tangent to world space
    //return normalize(vec3(1, 1, 1));
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

// 爱来自虚幻4，https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec3 SpecularIBL(vec3 SpecularColor, float Roughness, vec3 N, vec3 V)
{
    vec3 SpecularLighting = vec3(0);
    const uint NumSamples = 16;
    for(uint i = 0; i < NumSamples; i++)
    {
        vec2 Xi = hammersley2d(i, NumSamples);
        vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
        vec3 L = 2 * dot(V, H) * H - V;
        float NoV = clamp(dot(N, V), 0, 1);
        float NoL = clamp(dot(N, L), 0, 1);
        float NoH = clamp(dot(N, H), 0, 1);
        float VoH = clamp(dot(V, H), 0, 1);
        
        if (NoL > 0)
        {
            vec3 SampleColor = textureLod(skybox_specular_texture, L, 0).xyz;
            float G = G_SchlicksmithGGX(NoL, NoV, Roughness);
            vec3 F = F_Schlick(VoH, SpecularColor); // VoH == LoH
            
            /* 
            本来应该是 输出亮度 = 入射光light * 高光brdf / 概率密度pdf
            其中 pdf = D * NoH / (4 * VoH) 有D = D_GGX(NoH, Roughness)
            入射光 light = SampleColor * NoL
            高光brdf = D * G * F / (4 * NoL * NoV)
            
            化简后得到下面实际计算用的式子，连GGX都不用算了
            */
            SpecularLighting += SampleColor * F * G * VoH / (NoH * NoV);
        }
    }
    return SpecularLighting / NumSamples;
}

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
    // 施密特正交化
    const vec3 tangent = normalize(vs_out.tangent - normal * dot(normal, vs_out.tangent));
    // 这个B的方向和纹理坐标的方向有关，同时还要考虑贴图的副发现定义，F**K
    const vec3 bitangent = cross(tangent, normal);
    // const mat3 tbn = mat3(tangent, cross(tangent, normal), normal);
    vec3 h = texture(normal_texture, vs_out.tex_coords).xyz * 2 - 1;
    vec3 world_normal = tangent * h.x + bitangent * h.y + normal * h.z;
    return world_normal;
}

void main()
{
    const vec3 dielectric_specular = vec3(0.04);//一般电解质的基础反射率

    const vec3 N = caculate_normal();//法线
    const vec3 metallic_roughness = texture(metallic_roughness_texture, vs_out.tex_coords).xyz;
    const vec3 V = normalize(camera_position - vs_out.world_position); // 观察方向

    PixelArribute pixel_attribute;
    pixel_attribute.albedo = texture(basecolor_texture, vs_out.tex_coords).xyz;//基础色
    pixel_attribute.roughness = metallic_roughness.y * roughness_factor;//粗糙度
    pixel_attribute.metallic = metallic_roughness.x * metallic_factor;//金属度
    // 对于金属，其反射率就是albedo。对于一般电介质，其反射率取一般值0.04。不考虑半导体。
    // 反射率F用于计算Fresnel反射
    pixel_attribute.F0 = mix(dielectric_specular, pixel_attribute.albedo, pixel_attribute.metallic);

    // 环境光
    vec3 result_color = vec3(0);

    result_color += ambient_light * pixel_attribute.albedo;
    // IBL
    result_color += SpecularIBL(pixel_attribute.F0, pixel_attribute.roughness, N, V);

    // 点光源
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        vec3 L = normalize(light_pos - vs_out.world_position);

        float squared_distance = dot(light_pos - vs_out.world_position, light_pos - vs_out.world_position);

        vec3 point_light_indensity = pointlight_list[i].intensity / squared_distance * max(dot(L, N), 0.0f);
        
        result_color += point_light_indensity * BRDF(L, V, N, pixel_attribute);
    }

# ifdef GYA_FOG_EXP
    const vec3 fog_color = vec3(1.0f ,1.0f ,1.0f);
    float distance = max(0.0f, length(vs_out.world_position - camera_position) - fog_min_distance);
    float fog_factor = exp(-distance * fog_density);
    result_color = mix(fog_color, result_color, fog_factor);
# endif

    out_color = vec4(pow(result_color, vec3(1 / 2.2)), 1.0f);
}