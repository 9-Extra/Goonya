#pragma once

#include "core/cgmath.h"
#include "runtime/GoonyaException.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <initializer_list>
#include <string>
#include <unordered_map>

namespace Goonya::Meta {

enum class FieldType : uint32_t { nul, i32, i64, u32, u64, f32, f64, vec2f, vec3f, vec4f, mat4f };

template <FieldType>
struct FieldType2CType {
    using Type = void;
};

template <typename T>
struct CType2FieldType {
    const static FieldType Type = FieldType::nul;
};

#define GOONYA_DEFINE_FIELDTYPE2CTYPE(field_type, ctype)                                                               \
    template <>                                                                                                        \
    struct FieldType2CType<FieldType::field_type> {                                                                    \
        using Type = ctype;                                                                                            \
    };                                                                                                                 \
    template <>                                                                                                        \
    struct CType2FieldType<ctype> {                                                                                    \
        const static FieldType Type = FieldType::field_type;                                                           \
    };

GOONYA_DEFINE_FIELDTYPE2CTYPE(i32, int32_t)
GOONYA_DEFINE_FIELDTYPE2CTYPE(i64, int64_t)
GOONYA_DEFINE_FIELDTYPE2CTYPE(u32, uint32_t)
GOONYA_DEFINE_FIELDTYPE2CTYPE(u64, uint64_t)
GOONYA_DEFINE_FIELDTYPE2CTYPE(f32, float)
GOONYA_DEFINE_FIELDTYPE2CTYPE(f64, double)
GOONYA_DEFINE_FIELDTYPE2CTYPE(vec2f, Vector2f)
GOONYA_DEFINE_FIELDTYPE2CTYPE(vec3f, Vector3f)
GOONYA_DEFINE_FIELDTYPE2CTYPE(vec4f, Vector4f)
GOONYA_DEFINE_FIELDTYPE2CTYPE(mat4f, Matrix4)

#undef GOONYA_DEFINE_FIELDTYPE2CTYPE

template <typename T>
concept meta_type = CType2FieldType<T>::Type != FieldType::nul;

//---------------------------------------------------------

constexpr size_t sizeof_field_type(FieldType type) noexcept {
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

// 存储一个类型为FieldType的动态类型的值
struct DynamicData {
    DynamicData() : type(FieldType::nul) {}

    template <meta_type T>
    explicit DynamicData(const T &value) {
        type = CType2FieldType<T>::Type;
        if (is_internal()) {
            *(T *)&storage.value = value;
        } else {
            storage.ptr = malloc(size_bytes());
            *(T *)storage.ptr = value;
        }
    }

    DynamicData(const DynamicData &other) : type(other.type) {
        if (is_internal()) {
            storage.value = other.storage.value;
        } else {
            storage.ptr = malloc(size_bytes());
            memcpy(storage.ptr, other.storage.ptr, size_bytes());
        }
    }

    DynamicData(DynamicData &&other) noexcept : type(other.type) {
        storage = other.storage;
        other.type = FieldType::nul;
    }

    ~DynamicData() { reset(); }

    FieldType get_type() const noexcept { return this->type; }
    size_t size_bytes() const noexcept { return sizeof_field_type(type); }
    bool is_empty() const noexcept { return type == FieldType::nul; }
    void reset() noexcept {
        if (!is_internal()) {
            free(storage.ptr);
        }
        type = FieldType::nul;
    }
    template <meta_type T>
    DynamicData &operator=(const T &value) noexcept {
        reset();
        type = CType2FieldType<T>::Type;
        if (is_internal()) {
            *(T *)&storage.value = value;
        } else {
            storage.ptr = malloc(size_bytes());
            memcpy(storage.ptr, &value, size_bytes());
        }
        return *this;
    }

    DynamicData &operator=(const DynamicData &other) noexcept {
        reset();
        type = other.type;
        if (is_internal()) {
            storage.value = other.storage.value;
        } else {
            storage.ptr = malloc(size_bytes());
            memcpy(storage.ptr, other.storage.ptr, size_bytes());
        }
        return *this;
    }

    DynamicData &operator=(DynamicData &&other) noexcept {
        reset();
        type = other.type;
        storage = other.storage;
        other.type = FieldType::nul;
        return *this;
    }

    bool operator==(const DynamicData &other) const noexcept {
        if (this->type != other.type) {
            return false;
        }
        if (is_internal()) {
            return this->storage.value == other.storage.value; // 不考虑vaule所占位数问题
        } else {
            return memcmp(this->storage.ptr, this->storage.ptr, size_bytes());
        }
    }

    template <meta_type T>
    T &get_value() const {
        if (type != CType2FieldType<T>::Type) {
            throw RuntimeError(std::format("类型不匹配：{} 与 {}", type, CType2FieldType<T>::Type));
        }
        if (is_internal()) {
            return reinterpret_cast<T>(storage.value);
        } else {
            return *(T *)storage.ptr;
        }
    }

    void copy_to(void *dest) const noexcept {
        const void *ptr = is_internal() ? &storage.value : storage.ptr;
        memcpy(dest, ptr, size_bytes());
    }

private:
    bool is_internal() const noexcept {
        // FieldType::nul也算internal
        return size_bytes() <= sizeof(storage);
    }

    union Storage {
        void *ptr;
        void *value;
    } storage{};

    FieldType type;
};

struct FieldInfo {
    FieldType type;
    uint32_t offset;
};

struct LayoutInfo {
    std::unordered_map<std::string, FieldInfo> fields;
    uint32_t size = 0;

    static LayoutInfo init(uint32_t size, std::initializer_list<std::tuple<std::string, FieldType, uint32_t>> fields) {
        LayoutInfo layout;
        layout.size = size;
        for (const auto &[name, type, offset] : fields) {
            layout.fields.emplace(name, FieldInfo{type, offset});
        }
        return layout;
    }
};

class DynamicStructWriter {
private:
    const LayoutInfo &layout_info;
    uint8_t *ptr;

public:
    explicit DynamicStructWriter(const LayoutInfo &layout_info, void *ptr = nullptr)
        : layout_info(layout_info), ptr((uint8_t *)ptr) {}
    void set_base_ptr(void *ptr) noexcept { this->ptr = (uint8_t *)ptr; }

    size_t get_size() const noexcept { return layout_info.size; }
    uint8_t *get_ptr(const std::string &name) noexcept { return ptr + layout_info.fields.at(name).offset; }
    const uint8_t *get_ptr(const std::string &name) const noexcept { return ptr + layout_info.fields.at(name).offset; }

    template <Meta::meta_type T>
    void set_field(const std::string &name, const T &value) noexcept {
        auto field_info = layout_info.fields.at(name);
        assert(Meta::CType2FieldType<T>::Type == field_info.type); // 检测类型一致
        *(T *)(ptr + field_info.offset) = value;
    }

    void set_field(const std::string &name, const Meta::DynamicData &value) noexcept {
        auto field_info = layout_info.fields.at(name);
        assert(value.get_type() == field_info.type); // 检测类型一致
        value.copy_to(ptr + field_info.offset);
    }

    template <Meta::meta_type T>
    void set_if_exist(const std::string &name, const T &value) noexcept {
        if (auto iter = layout_info.fields.find(name); iter != layout_info.fields.end()) {
            auto field_info = iter->second;
            assert(Meta::CType2FieldType<T>::Type == field_info.type); // 检测类型一致
            *(T *)(ptr + field_info.offset) = value;
        }
    }

    void set_if_exist(const std::string &name, const Meta::DynamicData &value) noexcept {
        if (auto iter = layout_info.fields.find(name); iter != layout_info.fields.end()) {
            auto field_info = iter->second;
            assert(value.get_type() == field_info.type); // 检测类型一致
            value.copy_to(ptr + field_info.offset);
        }
    }

    template <Meta::meta_type T>
    T &operator[](const std::string &name) noexcept {
        assert(layout_info.fields.at(name).type == Meta::CType2FieldType<T>::Type);
        return *(T *)get_ptr(name);
    }

    template <Meta::meta_type T>
    const T &operator[](const std::string &name) const noexcept {
        assert(layout_info.fields.at(name).type == Meta::CType2FieldType<T>::Type);
        return *(T *)get_ptr(name);
    }

    bool contains(const std::string &name) const noexcept { return layout_info.fields.contains(name); }
};

} // namespace Goonya::Meta

template <>
struct std::formatter<Goonya::Meta::FieldType> {
    constexpr auto parse(std::format_parse_context &context) /* NOLINT*/ { return context.begin(); }
    constexpr auto format(const Goonya::Meta::FieldType t, std::format_context &ctx) const // NOLINT
    {
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
    constexpr auto parse(std::format_parse_context &context) /* NOLINT*/ { return context.begin(); }
    auto format(const Goonya::Meta::LayoutInfo &p, std::format_context &ctx) /* NOLINT*/ const {
        auto i = std::format_to(ctx.out(), "{{\n");
        ctx.advance_to(i);
        for (const auto &[name, f] : p.fields) {
            auto i = std::format_to(ctx.out(), "{}: {}\n", name, f.type);
            ctx.advance_to(i);
        }
        return std::format_to(ctx.out(), "}}");
    }
};