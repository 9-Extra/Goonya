#pragma once

#include "core/cgmath.h"
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace Goonya {
namespace Meta {

enum class FieldType : uint32_t { nul, i32, i64, u32, u64, f32, f64, vec2f, vec3f, vec4f, mat4f };

template <FieldType>
struct FieldType2CType {
    using Type = void;
};

template <typename T>
struct CType2FieldType {
    FieldType Type = FieldType::nul;
};

#define _GOONYA_DEFINE_FIELDTYPE2CTYPE(field_type, ctype)                                                              \
    template <>                                                                                                        \
    struct FieldType2CType<FieldType::field_type> {                                                                    \
        using Type = ctype;                                                                                            \
    };                                                                                                                 \
    template <>                                                                                                        \
    struct CType2FieldType<ctype> {                                                                                    \
        FieldType Type = FieldType::field_type;                                                                        \
    };

_GOONYA_DEFINE_FIELDTYPE2CTYPE(i32, int32_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(i64, int64_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(u32, uint32_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(u64, uint64_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(f32, float)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(f64, double)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(vec2f, Vector2f)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(vec3f, Vector3f)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(vec4f, Vector4f)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(mat4f, Matrix4)

//---------------------------------------------------------

inline size_t sizeof_field_type(FieldType type) noexcept {
    switch (type) {
    case FieldType::nul:
        return 0;
    case FieldType::i32:
        return sizeof(FieldType2CType<FieldType::i32>::Type);
    case FieldType::i64:
        return sizeof(FieldType2CType<FieldType::i64>::Type);
    case FieldType::u32:
        return sizeof(FieldType2CType<FieldType::u32>::Type);
    case FieldType::u64:
        return sizeof(FieldType2CType<FieldType::u64>::Type);
    case FieldType::f32:
        return sizeof(FieldType2CType<FieldType::f32>::Type);
    case FieldType::f64:
        return sizeof(FieldType2CType<FieldType::f64>::Type);
    case FieldType::vec2f:
        return sizeof(FieldType2CType<FieldType::vec2f>::Type);
    case FieldType::vec3f:
        return sizeof(FieldType2CType<FieldType::vec3f>::Type);
    case FieldType::vec4f:
        return sizeof(FieldType2CType<FieldType::vec4f>::Type);
    case FieldType::mat4f:
        return sizeof(FieldType2CType<FieldType::mat4f>::Type);
    }
    return 0;
};

struct Field {
    FieldType type;
    size_t offset;
};

struct LayoutInfo {
    std::unordered_map<std::string, Field> fields;
    size_t size;

    static LayoutInfo init(size_t size, std::initializer_list<std::tuple<std::string, FieldType, size_t>> fields) {
        LayoutInfo layout;
        layout.size = size;
        for (const auto &[name, type, offset] : fields) {
            layout.fields.emplace(name, Field{type, offset});
        }
        return layout;
    }
};

class DynamicStruct {
public:
    size_t size() const noexcept { return total_size; }

    uint8_t *get_ptr(const std::string &name) noexcept { return data + layout_info.fields.at(name).offset; }

    uint8_t *const get_ptr(const std::string &name) const noexcept { return data + layout_info.fields.at(name).offset; }

    template <class T>
    T &operator[](const std::string &name) noexcept {
        assert(layout_info.fields.at(name).type == CType2FieldType<T>());
        return *(T *)get_ptr(name);
    }

    template <class T>
    const T &operator[](const std::string &name) const noexcept {
        assert(layout_info.fields.at(name).type == CType2FieldType<T>());
        return *(T *)get_ptr(name);
    }

    bool contains(const std::string &name) const noexcept { return layout_info.fields.contains(name); }

private:
    friend class DynamicStructBuilder;
    LayoutInfo layout_info;
    uint8_t *data;
    size_t total_size;
};

class DynamicStructBuilder {
public:
    struct FieldRecord {
        std::string name;
        FieldType type;
    };

    DynamicStructBuilder() {}
    DynamicStructBuilder(std::initializer_list<FieldRecord> fields) noexcept : fields(fields) {}

    void append_field(const std::string &name, FieldType type) noexcept { fields.emplace_back(name, type); }

    void append_fields(std::initializer_list<FieldRecord> fields) noexcept {
        for (FieldRecord f : fields) {
            this->fields.emplace_back(f);
        }
    }

private:
    std::vector<FieldRecord> fields;
};

} // namespace Meta
} // namespace Goonya

template <>
struct std::formatter<Goonya::Meta::FieldType> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); }
    constexpr auto format(const Goonya::Meta::FieldType t, std::format_context &ctx) const {
        switch (t) {
        case Goonya::Meta::FieldType::nul:
            return std::format_to(ctx.out(), "nul");
        case Goonya::Meta::FieldType::i32:
            return std::format_to(ctx.out(), "i32");
        case Goonya::Meta::FieldType::i64:
            return std::format_to(ctx.out(), "i64");
        case Goonya::Meta::FieldType::u32:
            return std::format_to(ctx.out(), "u32");
        case Goonya::Meta::FieldType::u64:
            return std::format_to(ctx.out(), "u64");
        case Goonya::Meta::FieldType::f32:
            return std::format_to(ctx.out(), "f32");
        case Goonya::Meta::FieldType::f64:
            return std::format_to(ctx.out(), "f64");
        case Goonya::Meta::FieldType::vec2f:
            return std::format_to(ctx.out(), "vec2f");
        case Goonya::Meta::FieldType::vec3f:
            return std::format_to(ctx.out(), "vec3f");
        case Goonya::Meta::FieldType::vec4f:
            return std::format_to(ctx.out(), "vec4f");
        case Goonya::Meta::FieldType::mat4f:
            return std::format_to(ctx.out(), "mat4f");
        }
        return std::format_to(ctx.out(), "illegal");
    }
};

template <>
struct std::formatter<Goonya::Meta::LayoutInfo> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); }
    auto format(const Goonya::Meta::LayoutInfo &p, std::format_context &ctx) const {
        auto i = std::format_to(ctx.out(), "{{\n");
        ctx.advance_to(i);
        for (const auto &[name, f] : p.fields) {
            auto i = std::format_to(ctx.out(), "{}: {}\n", name, f.type);
            ctx.advance_to(i);
        }
        return std::format_to(ctx.out(), "}}");
    }
};