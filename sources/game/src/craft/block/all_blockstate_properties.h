#pragma once

#include "blockstate.h"
#include "craft/core/core.h"

#include <cassert>
#include <memory>
#include <optional>

namespace Craft {

// Property for BlockState
class Properties {
public:
    BlockStateProperty * const DIRECTION = _register(BlockStateProperty::create_enum<Direction>("direction"));
    BlockStateProperty * const NUMBER = _register(BlockStateProperty::create_int("number", {0, 1, 2}));
    BlockStateProperty * const BURNING = _register(BlockStateProperty::create_bool("burning"));

private:
    static std::optional<Properties> instance;

public:
    Properties() = default;
    Properties(Properties&) = delete;

    static Properties& get() noexcept {
        assert(instance.has_value());
        return instance.value();
    }

    static void initalize() {
        assert(!instance.has_value());
        instance.emplace();
    }

private:
    BlockStateProperty* _register(std::unique_ptr<BlockStateProperty>&& property) // NOLINT
    {
        assert(property && !PROPERTY_REGISTRY.contains(property.get()));
        
        std::string name{property->get_name()};
        BlockStateProperty* p = property.get();
        PROPERTY_REGISTRY.do_register(std::move(name), std::move(property));
        return p;
    }
};

} // namespace Craft