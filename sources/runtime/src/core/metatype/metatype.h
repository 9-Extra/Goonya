#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "core/cgmath.h"

namespace Goonya {
namespace Meta {

enum class FieldType: uint32_t { nul, i32, i64, u32, u64, f32, f64, vec3f, vec4f, mat4f };

template <decltype(FieldType::nul)>
struct FieldType2CType{
    using Type=void;
};

#define _GOONYA_DEFINE_FIELDTYPE2CTYPE(field_type, ctype) \
template <>\
struct FieldType2CType<FieldType::field_type>{\
    using Type=ctype;\
};\

_GOONYA_DEFINE_FIELDTYPE2CTYPE(i32, int32_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(i64, int64_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(u32, uint32_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(u64, uint64_t)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(f32, float)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(f64, double)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(vec3f, Vector3f)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(vec4f, Vector4f)
_GOONYA_DEFINE_FIELDTYPE2CTYPE(mat4f, Matrix4)

// template<class T>
// constexpr FieldType ctype2fieldtype(){
//     if constexpr (std::is_same_v<T, int32_t>){
//         return FieldType::i32;
//     } else if constexpr (std::is_same_v<T, int64_t>){
//         return FieldType::i64;
//     } else if constexpr (std::is_same_v<T, uint32_t>){
//         return FieldType::u32;
//     } else if constexpr (std::is_same_v<T, uint64_t>){
//         return FieldType::u64;
//     } else if constexpr (std::is_same_v<T, float>){
//         return FieldType::f32;
//     } else if constexpr (std::is_same_v<T, double>){
//         return FieldType::f64;
//     } else if constexpr (std::is_same_v<T, Vector3f>){
//         return FieldType::vec3f;
//     } else if constexpr (std::is_same_v<T, Vector4f>){
//         return FieldType::vec4f;
//     } else if constexpr (std::is_same_v<T, Matrix>){
//         return FieldType::mat4f;
//     } 

//     return FieldType::nul;
// }

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

class DynamicStruct {
public:
    size_t size() const noexcept{
        return total_size;
    }

    uint8_t* get_ptr(const std::string& name) noexcept{
        return data + fields.at(name).offset;
    }

    uint8_t* const get_ptr(const std::string& name) const noexcept{
        return data + fields.at(name).offset;
    }
    
    template<class T>
    T& operator[](const std::string& name) noexcept{
        //assert(fields.at(name).type == ctype2fieldtype<T>());
        return *(T*)get_ptr(name);
    }

    template<class T>
    const T& operator[](const std::string& name) const noexcept{
        //assert(fields.at(name).type == ctype2fieldtype<T>());
        return *(T*)get_ptr(name);
    }

    bool contains(const std::string& name) const noexcept{
        return fields.contains(name);
    }

private:
    friend class DynamicStructBuilder;
    std::unordered_map<std::string, Field> fields;
    uint8_t* data;
    size_t total_size;
};

struct DynamicStructBuilder{
    struct FieldRecord{
        std::string name;
        FieldType type;
    };
    std::vector<FieldRecord> fields;

    void append_field(const std::string& name, FieldType type) noexcept{
        fields.emplace_back(name, type);
    }

    DynamicStruct build_opengl_std140() const;
};

} // namespace Meta
} // namespace Goonya