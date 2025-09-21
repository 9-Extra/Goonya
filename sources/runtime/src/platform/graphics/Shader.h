#pragma once

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include <cassert>
#include <string>
#include <unordered_map>

namespace Goonya::Graphics {

enum class BufferBindingType {
    UNIFORM,
    SHADER_STORAGE
};

struct ShaderUniformBlockInfo final {
    Meta::LayoutInfo layout;
    uint32_t binding = 0;
    BufferBindingType binding_type = BufferBindingType::UNIFORM;
};

class Shader : public intrusive_ptr_base<Shader> {
public:
    Shader(const Shader &) = delete;
    Shader(Shader &&) = delete;

    virtual void bind() = 0;
    virtual ~Shader() = default;

protected:
    Shader() = default;
};

class ShaderIntrospector {
public:
    virtual ~ShaderIntrospector() = default;

    virtual std::unordered_map<std::string, ShaderUniformBlockInfo> get_constant_buffer_info() const noexcept = 0;
    virtual std::unordered_map<std::string, uint32_t> get_texture_info() const noexcept = 0;

protected:
    ShaderIntrospector() = default;
};

} // namespace Goonya::Graphics
