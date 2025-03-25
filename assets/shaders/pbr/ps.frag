#version 440 core

#define POINTLIGNT_MAX 8

#pragma GYA_INJECT


struct PointLight{
    vec3 position; // 0
    vec3 intensity; // 4
};

// 帧相关参数和纹理
layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix; //16 * 4
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

layout(binding = 5) uniform samplerCube skybox_specular_texture;

// 材质参数和纹理
layout(binding = 2) uniform per_material
{
    float metallic_factor;
    float roughness_factor;
};

layout(binding = 0) uniform sampler2D basecolor_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 2) uniform sampler2D metallic_roughness_texture;


in VS_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

#define PI 3.1415926

struct PixelArribute{
    vec3 albedo;
    vec3 emission; // 自发光颜色
    float roughness;
    float metallic;
    vec3 F0;
}; 

// RTR4 9.41 P340，GGX分布
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

// Fresnel function ----------------------------------------------------
// 使用基础反射率F0和入射角近似计算反射率F
float Pow5(float x)
{
    return (x * x * x * x * x);
}
// 这是一种拟合方式
vec3 F_Schlick(float cosTheta, vec3 F0) 
{ 
    return F0 + (1.0 - F0) * Pow5(1.0 - cosTheta); 
}
// 这是另外一种
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * Pow5(1.0 - cosTheta);
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

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
    // 施密特正交化
    const vec3 tangent = normalize(vs_out.tangent - normal * dot(normal, vs_out.tangent));
    const mat3 tbn = mat3(tangent, cross(tangent, normal), normal);
    vec3 texture_value = texture(normal_texture, vs_out.tex_coords).xyz * 2 - 1;
    return tbn * texture_value;
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

#ifdef GYA_IBL_ENVIRONMENT_LIGHT
    vec3 L = 2 * dot(V, N) * N - V;
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    vec3 environment_light_indensity = textureLod(skybox_specular_texture, L, pixel_attribute.roughness * textureQueryLevels(skybox_specular_texture)).xyz;
    result_color += environment_light_indensity * G_SchlicksmithGGX(dotNL, dotNV, pixel_attribute.roughness) * F_Schlick(dotNV, pixel_attribute.F0);
#endif

    // 点光源
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        vec3 L = normalize(light_pos - vs_out.world_position);

        float squared_distance = dot(light_pos - vs_out.world_position, light_pos - vs_out.world_position);

        vec3 point_light_indensity = pointlight_list[i].intensity / squared_distance * max(dot(L, N), 0.0f);
        
        result_color += point_light_indensity * BRDF(L, V, N, pixel_attribute);
        //result_color = light_intensity;
    }

    result_color = min(result_color, 1.0f);
# ifdef GYA_FOG_EXP
    const vec3 fog_color = vec3(1.0f ,1.0f ,1.0f);
    float distance = max(0.0f, length(vs_out.world_position - camera_position) - fog_min_distance);
    float fog_factor = exp(-distance * fog_density);
    result_color = mix(fog_color, result_color, fog_factor);
# endif
    out_color = vec4(pow(result_color, vec3(1 / 2.2)), 1.0f);
    
    //out_color = vec4(BRDF(L, V, N, pixel_attribute) / 2, 1);
}