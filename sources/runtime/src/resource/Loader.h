#pragma once

#include "resource/Resource.h"
#include "json/value.h"
#include <filesystem>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace Goonya {

class ResourceLoader {  
public:  
    ResourceLoader(std::initializer_list<std::string_view> supported_types) {
        this->supported_types.reserve(supported_types.size());
        for(auto type: supported_types){
            this->supported_types.emplace_back(type);
        }
    }
    ResourceLoader(const ResourceLoader&) = delete;
protected:
    friend class RenderResource;
    std::vector<std::string> supported_types;
protected:
    virtual Ref<Resource> load(std::string_view type, const std::filesystem::path& base_dir, std::string_view name, const Json::Value& content) = 0;
};

void register_all_loaders();

}