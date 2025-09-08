#pragma once

#include "block.h"
#include "craft/core/registry.h"

#include <cassert>
#include <optional>

namespace Craft {

class Blocks {
public:
    Block *const AIR = _register("air", new Block("空气"));
    Block *const STONE = _register("stone", new Block("石头"));
    Block *const DIRT = _register("granite", new Block("花岗岩"));
    Block *const GRASS = _register("polished_granite", new Block("平滑花岗岩"));
private:
    static std::optional<Blocks> instance;

public:
    Blocks() // 不要调用，不知道为什么不能声明为private 
    { 
        assert(!instance.has_value());
    }
    Blocks(Blocks &) = delete;
    Blocks(Blocks &&) = delete;

    static const Blocks &get() noexcept {
        assert(instance.has_value());
        return instance.value();
    }

    static void initalize() { instance.emplace(); }

private:    
    Block *_register(std::string key, Block *block) noexcept // NOLINT: 不需要static
    {
        assert(block);
        REGISTRY_BLOCK.do_register(std::move(key), std::unique_ptr<Block>(block));
        return block;
    }
};

} // namespace Craft

