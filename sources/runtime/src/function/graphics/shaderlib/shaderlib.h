#pragma once

#include <cassert>
#include <glad/glad.h>
#include <string>
#include <unordered_map>


#include "function/graphics/opengl_utils.h"
#include "platform/read_file.h"
#include "core/metatype/metatype.h"

// namespace Goonya{
// namespace Graphics {

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

namespace Goonya {
namespace Graphics {

struct ShaderDesc {
    ShaderDesc() noexcept : hash_cache(), uber_name(), definations(){};

    ShaderDesc(const std::string &uber_name, std::unordered_map<std::string, std::string> &&definations) noexcept
        : uber_name(uber_name), definations(definations) {
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
    friend struct std::hash<Goonya::Graphics::ShaderDesc>;
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

} // namespace Graphics
} // namespace Goonya

template <>
struct std::hash<Goonya::Graphics::ShaderDesc> {
    size_t operator()(const Goonya::Graphics::ShaderDesc &desc) const noexcept { return desc.hash_cache; }
};

namespace Goonya {
namespace Graphics {

// 一个可写的uniform buffer对象的封装
template <class T> class FixedUniformBufferWriter{
public:
    FixedUniformBufferWriter(GLuint id) noexcept: id(id){
        ptr = (T*)glMapNamedBuffer(id, GL_WRITE_ONLY);
        assert(ptr != nullptr);
    }
    FixedUniformBufferWriter(FixedUniformBufferWriter& other) = delete;

    T* operator->() noexcept{
        return ptr;
    }

    ~FixedUniformBufferWriter() noexcept{
        bool ret = glUnmapNamedBuffer(id);
        assert(ret);
    }
private:
    GLuint id;
    T* ptr;
};

template <class T> struct FixedUniformBuffer {
    // 在初始化opengl后才能初始化
    FixedUniformBuffer() noexcept{
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    }

    FixedUniformBufferWriter<T> map() noexcept{
        return FixedUniformBufferWriter<T>(id);
    }

    void bind(unsigned int binding_point) const noexcept{
        glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, id);
    }

    ~FixedUniformBuffer() noexcept{
        glDeleteBuffers(1, &id);
    }

private:
    unsigned int id;
};

struct ConstantBufferInfo{
    struct FieldInfo{
        Meta::FieldType type;
        size_t offset;
    };
    std::string name;
    size_t total_size;
    std::unordered_map<std::string, FieldInfo> fields;
};

class DynamicUniformBufferWriter{
public:
    DynamicUniformBufferWriter(DynamicUniformBufferWriter& other) = delete;

    uint8_t* get_ptr(const std::string& name) noexcept{
        return ptr + info.fields.at(name).offset;
    }

    uint8_t* const get_ptr(const std::string& name) const noexcept{
        return ptr + info.fields.at(name).offset;
    }
    
    template<class T>
    T& operator[](const std::string& name) noexcept{
        //assert(info.fields.at(name).type == ctype2fieldtype<T>());
        return *(T*)get_ptr(name);
    }

    template<class T>
    const T& operator[](const std::string& name) const noexcept{
        //assert(info.fields.at(name).type == ctype2fieldtype<T>());
        return *(T*)get_ptr(name);
    }

    bool contains(const std::string& name) const noexcept{
        return info.fields.contains(name);
    }

    ~DynamicUniformBufferWriter() noexcept{
        glUnmapNamedBuffer(id);
        checkError();
    }
private:
    friend class DynamicUniformBuffer;
    DynamicUniformBufferWriter(GLuint id, ConstantBufferInfo& info) noexcept: id(id), info(info) {
        ptr = (uint8_t *)glMapNamedBuffer(id, GL_WRITE_ONLY);
        assert(ptr != nullptr);
    }
    uint8_t *ptr;
    GLuint id;
    ConstantBufferInfo& info;
};

class DynamicUniformBuffer {
    // 在初始化opengl后才能初始化
    DynamicUniformBuffer(ConstantBufferInfo&& info) noexcept: info(std::move(info)) {
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, info.total_size, nullptr, GL_DYNAMIC_DRAW);
    }

    DynamicUniformBufferWriter map() noexcept{
        return DynamicUniformBufferWriter(id, info);
    }

    void bind(GLuint binding_point) const noexcept{
        glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, id);
    }

    ~DynamicUniformBuffer(){
        glDeleteBuffers(1, &id);
    }

private:
    GLuint id;
    ConstantBufferInfo info;
};

class ShaderIntrospector {
public:
    ShaderIntrospector(GLuint program_id) noexcept : id(program_id) {}

    std::vector<std::tuple<std::string, ConstantBufferInfo>> get_constant_buffer_info() const noexcept;

private:
    GLuint id;
};

struct UberShaderDesc {
    std::string vs_path;
    std::string ps_path;
};

struct ShaderResource {
    GLuint gl_id;
};

class ShaderLib {
public:
    void add_uber_shader(const std::string &name, const UberShaderDesc &desc) {
        assert(!uber_shader_sources.contains(name));
        uber_shader_sources.emplace(name,
                                    UberShaderSource{read_whole_file(desc.vs_path), read_whole_file(desc.ps_path)});
    }
    ShaderResource query_shader(const ShaderDesc &desc) {
        assert(!desc.get_uber_name().empty());
        auto iter = shader_cache.find(desc);
        if (iter != shader_cache.end()) {
            return iter->second;
        }

        ShaderResource r = load_shader(desc);
        shader_cache.emplace(desc, r);

        return r;
    }

    void drop() {
        for (const auto &[k, v] : shader_cache) {
            glDeleteProgram(v.gl_id);
        }
        shader_cache.clear();
        uber_shader_sources.clear();
    }

private:
    struct UberShaderSource {
        std::string vs_src;
        std::string ps_src;
    };
    std::unordered_map<std::string, UberShaderSource> uber_shader_sources;
    std::unordered_map<ShaderDesc, ShaderResource> shader_cache;

    ShaderResource load_shader(const ShaderDesc &desc);
};


} // namespace Graphics
} // namespace Goonya