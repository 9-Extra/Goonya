#pragma once

#include <cassert>
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform/read_file.h"
#include "core/metatype/metatype.h"
#include "resource/resources.h"
#include "platform/graphics/graphics.h"

namespace Goonya {
namespace Graphics {

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

struct ShaderResource {
    GLuint gl_id;
    // std::vector<std::tuple<std::string, Meta::FieldType>> vertex_input;
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