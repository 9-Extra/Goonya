#pragma once
#include "core/metatype/metatype.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Shader.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Resource {

class PSOBuilder {
public:
    PSOBuilder(const std::string &shader_name): uber_shader_name(shader_name) {}

    PSOBuilder &set_variant_keys(std::unordered_set<std::string> variant_keys) {
        variant_keys.merge(std::move(variant_keys));
        return *this;
    }

    PSOBuilder &set_variant_key(const std::string &key) {
        variant_keys.emplace(key);
        return *this;
    }

    PSOBuilder &remove_variant_key(const std::string &key){
        if (auto iter = variant_keys.find(key);iter != variant_keys.end()){
            variant_keys.erase(iter);
        }
        return *this;
    }

    PSOBuilder &enable_cilp(bool enable = true) {
        b_enable_cilp = enable;
        return *this;
    }

    PSOBuilder &set_front_face_clockwise(const std::string &clockwise) {
        front_face_clockwise = clockwise;
        return *this;
    }

    PSOBuilder &enable_depth_test(bool enable = true) {
        b_enable_depth_test = enable;
        return *this;
    }

    PSOBuilder &set_depth_func(const std::string &depth_func) {
        s_depth_func = depth_func;
        return *this;
    }

    Graphics::PSODesc build() {
        if (uber_shader_name == "") {
            throw RuntimeError("着色器名称不应为空");
        }

        return Graphics::PSODesc{.shader_desc = Graphics::ShaderDesc{std::move(uber_shader_name), std::move(variant_keys)},
                       .enable_cilp = b_enable_cilp,
                       .cull_face_mode = std::move(cull_face_mode),
                       .front_face_clockwise = std::move(front_face_clockwise),
                       .enable_depth_test = b_enable_depth_test,
                       .depth_func = std::move(s_depth_func)};
    }

private:
    std::string uber_shader_name;
    std::unordered_set<std::string> variant_keys;
    bool b_enable_cilp = true;
    std::string cull_face_mode = "back";
    std::string front_face_clockwise = "counterclockwise";

    bool b_enable_depth_test = true;
    std::string s_depth_func = "less"; // glDepthFunc
};

class MaterialBuilder {
public:
    MaterialBuilder(const Graphics::PSODesc &pso) {
        desc.pso_desc = pso;
    }

    template<Meta::meta_type T>
    MaterialBuilder &add_parameter(const std::string& name, const T& vaule) {
        desc.parameters.emplace(name, vaule);
        return *this;
    }

    MaterialBuilder &add_sampler(const std::string& name, std::string texture_key) {
        desc.textures.emplace_back(name, texture_key);
        return *this;
    }

    Graphics::MaterialDesc build(){
        return desc;
    }

private:
    Graphics::MaterialDesc desc;
};

} // namespace Resource
} // namespace Goonya