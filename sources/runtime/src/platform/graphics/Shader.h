#pragma once

#include "core/cgmath.h"
#include "core/hash_helper.h"
#include "core/intrusive_ptr.h"
#include "platform/read_file.h"
#include <cassert>
#include <string>
#include <unordered_set>

namespace Goonya {
namespace Graphics {

struct ShaderDesc {
    std::string uber_name;
    std::unordered_set<std::string> variant_keys;

    ShaderDesc() {}

    template <typename T1, typename T2>
    ShaderDesc(T1 &&uber_name, T2 &&variant_keys) noexcept : uber_name(uber_name), variant_keys(variant_keys) {
        assert(!uber_name.empty());
    }

    ShaderDesc(const ShaderDesc &desc) noexcept : uber_name(desc.uber_name), variant_keys(desc.variant_keys) {}
    ShaderDesc(ShaderDesc &&desc) noexcept
        : uber_name(std::move(desc.uber_name)), variant_keys(std::move(desc.variant_keys)) {}

    ShaderDesc &operator=(const ShaderDesc &desc) noexcept = default;
    bool operator==(const ShaderDesc &b) const noexcept = default;

private:
    friend struct std::hash<Goonya::Graphics::ShaderDesc>;
    size_t hash() const noexcept {
        size_t seed = std::hash<std::string>{}(uber_name);
        for (const std::string &key : variant_keys) {
            hash_combine(seed, key);
        }
        return seed;
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

} // namespace Graphics
} // namespace Goonya
template <>
struct std::hash<Goonya::Graphics::ShaderDesc> {
    size_t operator()(const Goonya::Graphics::ShaderDesc &desc) const noexcept { return desc.hash(); }
};

template <>
struct std::hash<Goonya::Graphics::PSODesc> {
    size_t operator()(const Goonya::Graphics::PSODesc &desc) const noexcept {
        size_t seed = 0;
        Goonya::hash_combine(seed, desc.shader_desc);
        Goonya::hash_combine(seed, desc.cull_face_mode);
        Goonya::hash_combine(seed, desc.front_face_clockwise);
        Goonya::hash_combine(seed, desc.depth_func);
        return seed ^ desc.enable_depth_test << 3 ^ desc.enable_cilp;
    }
};

namespace Goonya {
namespace Graphics {

struct Vertex {
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f uv;
};

struct UberShaderDesc {
    std::string vs_path;
    std::string ps_path;
};

class Shader : public intrusive_ptr_base<Shader> {
public:
    virtual void bind() = 0;
    virtual ~Shader() = default;
};

class PipelineStateObject : public intrusive_ptr_base<PipelineStateObject> {
public:
    virtual void bind() const = 0;
    virtual ~PipelineStateObject() = default;

protected:
    PipelineStateObject() {};
};

class ShaderLib {
public:
    virtual ~ShaderLib() = default;

    void add_uber_shader(const std::string &name, const UberShaderDesc &desc) {
        assert(!uber_shader_sources.contains(name));
        uber_shader_sources.emplace(name,
                                    UberShaderSource{read_whole_file(desc.vs_path), read_whole_file(desc.ps_path)});
    }

    intrusive_ptr<Shader> query_shader(const ShaderDesc &desc) {
        assert(!desc.uber_name.empty());
        if (auto iter = shader_cache.find(desc); iter != shader_cache.end()) {
            return iter->second;
        }

        intrusive_ptr<Shader> r = load_shader(desc);
        shader_cache.emplace(desc, r);

        return r;
    }

    intrusive_ptr<PipelineStateObject> query_pso(const PSODesc &desc) {
        if (auto iter = pso_cache.find(desc); iter != pso_cache.end()) {
            return iter->second;
        }
        intrusive_ptr<PipelineStateObject> r = load_pso(desc);
        pso_cache.emplace(desc, r);
        return r;
    }


protected:
    virtual intrusive_ptr<Shader> load_shader(const ShaderDesc &desc) = 0;
    virtual intrusive_ptr<PipelineStateObject> load_pso(const PSODesc &desc) = 0;


    struct UberShaderSource {
        std::string vs_src;
        std::string ps_src;
    };
    std::unordered_map<std::string, UberShaderSource> uber_shader_sources;
    std::unordered_map<ShaderDesc, intrusive_ptr<Shader>> shader_cache;
    std::unordered_map<PSODesc, intrusive_ptr<PipelineStateObject>> pso_cache;
};

} // namespace Graphics
} // namespace Goonya
