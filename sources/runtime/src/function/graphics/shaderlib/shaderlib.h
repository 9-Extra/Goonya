#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <cassert>

#include "platform/read_file.h"

//namespace Goonya{
//namespace Graphics {

// struct ShaderDefine{
//     // #define def_name def_val;
//     std::string def_name;
//     std::string def_val;

//     bool operator==(const ShaderDefine& b) const noexcept{
//         return def_name == b.def_name && def_val == b.def_val;
//     }
// };
// }
// }

// template <>
// struct std::hash<Goonya::Graphics::ShaderDefine>{
//     size_t operator()(const Goonya::Graphics::ShaderDefine& def) const noexcept{
//         static auto hasher = std::hash<std::string>();
//         return hasher(def.def_name) ^ (hasher(def.def_val) << 1);
//     }
// };

namespace Goonya{
namespace Graphics {

struct ShaderDesc{
    ShaderDesc() noexcept:uber_name(), definations(), hash_cache(){};

    ShaderDesc(const std::string& uber_name, std::unordered_map<std::string, std::string>&& definations) noexcept
    : uber_name(uber_name), definations(definations)
    {
        assert(!uber_name.empty());
        hash_cache = hash();
    }

    ShaderDesc(const ShaderDesc& desc) noexcept :uber_name(desc.uber_name), definations(desc.definations), hash_cache(desc.hash_cache) {}
    ShaderDesc(ShaderDesc&& desc) noexcept :uber_name(std::move(desc.uber_name)), definations(std::move(desc.definations)), hash_cache(desc.hash_cache) {}

    ShaderDesc& operator=(const ShaderDesc& desc) noexcept = default;

    const std::string& get_uber_name() const noexcept{
        return uber_name;
    }
    const std::unordered_map<std::string, std::string>& get_definations() const noexcept{
        return definations;
    }
    
    bool operator==(const ShaderDesc& b) const noexcept{
        return hash_cache == b.hash_cache && uber_name == b.uber_name && definations == b.definations;
    }

private:
    friend struct std::hash<Goonya::Graphics::ShaderDesc>;
    size_t hash_cache;
    std::string uber_name;
    std::unordered_map<std::string, std::string> definations;

    size_t hash() const noexcept{
        size_t result = std::hash<std::string>{}(uber_name);
        for(const auto& [k, v] : definations){
            result ^= std::hash<std::string>{}(k) ^ std::hash<std::string>{}(v);
        }
        return result;
    }
};

}
}

template <>
struct std::hash<Goonya::Graphics::ShaderDesc>{
    size_t operator()(const Goonya::Graphics::ShaderDesc& desc) const noexcept{
        return desc.hash_cache;
    }
};

namespace Goonya{
namespace Graphics {

struct UberShaderDesc{
    std::string vs_path;
    std::string ps_path;
};

struct ShaderResource{
    GLuint gl_id;
};

class ShaderLib{
public:
    void add_uber_shader(const std::string& name, const UberShaderDesc& desc){
        assert(!uber_shader_sources.contains(name));
        uber_shader_sources.emplace(name, UberShaderSource{read_whole_file(desc.vs_path), read_whole_file(desc.ps_path)});
    }
    ShaderResource query_shader(const ShaderDesc& desc){
        assert(!desc.get_uber_name().empty());
        auto iter = shader_cache.find(desc);
        if (iter != shader_cache.end()){
            return iter->second;
        }

        ShaderResource r = load_shader(desc);
        shader_cache.emplace(desc, r);

        return r;
    }

    void drop(){
        for(const auto& [k, v] : shader_cache){
            glDeleteProgram(v.gl_id);
        }   
        shader_cache.clear();
        uber_shader_sources.clear();
    }
private:
    struct UberShaderSource{
        std::string vs_src;
        std::string ps_src;
    };
    std::unordered_map<std::string, UberShaderSource> uber_shader_sources;
    std::unordered_map<ShaderDesc, ShaderResource> shader_cache;

    ShaderResource load_shader(const ShaderDesc& desc);
};

}
}