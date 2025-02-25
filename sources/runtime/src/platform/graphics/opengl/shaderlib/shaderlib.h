#pragma once

#include <cassert>
#include <glad/glad.h>
#include <string>
#include <unordered_map>

#include "platform/read_file.h"
#include "core/metatype/metatype.h"
#include "resource/resources.h"
#include "platform/graphics/graphics.h"

namespace Goonya {
namespace Graphics {

class ShaderIntrospector {
public:
    ShaderIntrospector(GLuint program_id) noexcept : id(program_id) {}

    std::unordered_map<std::string, Meta::LayoutInfo> get_constant_buffer_info() const noexcept;

private:
    GLuint id;
};

struct ShaderResource {
    GLuint gl_id;
    // std::vector<std::tuple<std::string, Meta::FieldType>> vertex_input;
    Meta::LayoutInfo per_object;
    Meta::LayoutInfo per_material;
    Meta::LayoutInfo per_frame;
     
};

class ShaderLib {
public:
    ~ShaderLib() {
        for (const auto &[k, v] : shader_cache) {
            glDeleteProgram(v.gl_id);
        }
        shader_cache.clear();
        uber_shader_sources.clear();
    }
    
    void add_uber_shader(const std::string &name, const UberShaderDesc &desc) {
        assert(!uber_shader_sources.contains(name));
        uber_shader_sources.emplace(name,
                                    UberShaderSource{read_whole_file(desc.vs_path), read_whole_file(desc.ps_path)});
    }
    ShaderResource query_shader(const Resource::ShaderDesc &desc) {
        assert(!desc.get_uber_name().empty());
        auto iter = shader_cache.find(desc);
        if (iter != shader_cache.end()) {
            return iter->second;
        }

        ShaderResource r = load_shader(desc);
        shader_cache.emplace(desc, r);

        return r;
    }


private:
    struct UberShaderSource {
        std::string vs_src;
        std::string ps_src;
    };
    std::unordered_map<std::string, UberShaderSource> uber_shader_sources;
    std::unordered_map<Resource::ShaderDesc, ShaderResource> shader_cache;

    ShaderResource load_shader(const Resource::ShaderDesc &desc);
};


} // namespace Graphics
} // namespace Goonya