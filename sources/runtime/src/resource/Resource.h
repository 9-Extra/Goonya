#pragma once


#include <json/config.h>
#include <json/json.h>
#include <json/value.h>
#include <string>
#include <unordered_map>

#include "core/RefCount.h"
#include "core/hash_helper.h"

namespace Goonya {

using AssetKey = std::string;

class Resource: public RefCount{
public:
    virtual bool is_pack() const noexcept{
        return false;
    }
};

class ResourcePack: public Resource{
public:
    std::unordered_map<AssetKey, Ref<Resource>, StringHash, StringEqual> contents;
    bool is_pack() const noexcept override {
        return true;
    }
};

} // namespace Goonya::Resource
