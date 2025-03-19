#pragma once
#include "core/metatype/metatype.h"
#include "resources.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Resource {

class PSOBuilder {
public:
    PSOBuilder(const std::string &shader_name): uber_shader_name(shader_name) {}

    PSOBuilder &set_shader_define(const std::string &key, const std::string &value = "") {
        shader_define[key] = value;
        return *this;
    }

    PSOBuilder &update_shader_defines(const std::unordered_map<std::string, std::string>& shader_define){
        this->shader_define.insert(shader_define.begin(), shader_define.end());
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

    PSODesc build() {
        if (uber_shader_name == "") {
            throw RuntimeError("着色器名称不应为空");
        }

        return PSODesc{.shader_desc = ShaderDesc{std::move(uber_shader_name), std::move(shader_define)},
                       .enable_cilp = b_enable_cilp,
                       .cull_face_mode = std::move(cull_face_mode),
                       .front_face_clockwise = std::move(front_face_clockwise),
                       .enable_depth_test = b_enable_depth_test,
                       .depth_func = std::move(s_depth_func)};
    }

private:
    std::string uber_shader_name;
    std::unordered_map<std::string, std::string> shader_define;
    bool b_enable_cilp = true;
    std::string cull_face_mode = "back";
    std::string front_face_clockwise = "counterclockwise";

    bool b_enable_depth_test = true;
    std::string s_depth_func = "less"; // glDepthFunc
};

class MaterialBuilder {
public:
    MaterialBuilder(const PSODesc &pso) {
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

    MaterialDesc build(){
        return desc;
    }

private:
    MaterialDesc desc;
};

} // namespace Resource
} // namespace Goonya