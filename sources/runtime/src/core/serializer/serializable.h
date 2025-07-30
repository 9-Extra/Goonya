#pragma once

#include "json/value.h"
namespace Goonya::Utils {

class Serializable{
    virtual ~Serializable() = default;
    virtual Json::Value to_json() const = 0;
    virtual Json::Value from_json() const = 0;
};

}