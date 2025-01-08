#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Goonya {
namespace Resource {

struct TextureDesc {
    std::string path;
};

struct ShaderDesc {
    ShaderDesc() noexcept : hash_cache(), uber_name(), definations() {};

    template <typename T1, typename T2>
    ShaderDesc(T1 &&uber_name, T2 &&definations) noexcept : uber_name(uber_name), definations(definations) {
        assert(!uber_name.empty());
        hash_cache = hash();
    }

    ShaderDesc(const ShaderDesc &desc) noexcept
        : hash_cache(desc.hash_cache), uber_name(desc.uber_name), definations(desc.definations) {}
    ShaderDesc(ShaderDesc &&desc) noexcept
        : hash_cache(desc.hash_cache), uber_name(std::move(desc.uber_name)), definations(std::move(desc.definations)) {}

    ShaderDesc &operator=(const ShaderDesc &desc) noexcept = default;

    const std::string &get_uber_name() const noexcept { return uber_name; }
    const std::unordered_map<std::string, std::string> &get_definations() const noexcept { return definations; }

    bool operator==(const ShaderDesc &b) const noexcept {
        return hash_cache == b.hash_cache && uber_name == b.uber_name && definations == b.definations;
    }

private:
    friend struct std::hash<Goonya::Resource::ShaderDesc>;
    size_t hash_cache;
    std::string uber_name;
    std::unordered_map<std::string, std::string> definations;

    size_t hash() const noexcept {
        size_t result = std::hash<std::string>{}(uber_name);
        for (const auto &[k, v] : definations) {
            result ^= std::hash<std::string>{}(k) ^ std::hash<std::string>{}(v);
        }
        return result;
    }
};

struct PSODesc {
    ShaderDesc shader_desc;

    bool enable_cilp;                 // glEnable(GL_CULL_FACE)
    std::string cull_face_mode;       // glCullFace
    std::string front_face_clockwise; // glFrontFace

    bool enable_depth_test; // glEnable(GL_DEPTH_TEST)
    std::string depth_func; // glDepthFunc

    bool operator==(const PSODesc &b) const noexcept = default;
};

struct MaterialDesc {
    struct UniformDataDesc {
        const uint32_t binding_id;
        const uint32_t size;
        const void *data = nullptr;
    };

    struct SampleData {
        uint32_t binding_id;
        std::string texture_key;
        std::string texture_type;
        std::string warp_mode;
        std::string filter_mode;
    };

    PSODesc pso_desc;
    std::vector<UniformDataDesc> uniforms;
    std::vector<SampleData> samplers;
};

} // namespace Resource
} // namespace Goonya

template <>
struct std::hash<Goonya::Resource::ShaderDesc> {
    size_t operator()(const Goonya::Resource::ShaderDesc &desc) const noexcept { return desc.hash_cache; }
};

template <>
struct std::hash<Goonya::Resource::PSODesc> {
    size_t operator()(const Goonya::Resource::PSODesc &desc) const noexcept {
        return std::hash<decltype(desc.shader_desc)>{}(desc.shader_desc) &
               std::hash<decltype(desc.cull_face_mode)>{}(desc.cull_face_mode) &
               std::hash<decltype(desc.front_face_clockwise)>{}(desc.front_face_clockwise) &
               std::hash<decltype(desc.depth_func)>{}(desc.depth_func) & desc.enable_depth_test << 3 & desc.enable_cilp;
    }
};