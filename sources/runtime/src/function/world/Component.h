#pragma once

#include <cassert>

#include "core/enum_operator.h"

namespace Goonya {
class GObject;

enum class ComponentUpdateFlag{
    NONE = 0,
    TRANSFORM = 1,
};

DECLARE_ENUM_OPERATORS(ComponentUpdateFlag);

/**
 * @brief 组件基类
 * 
 * 组件挂在GObject上，在每个tick时会被调用，不同的组件通过重载tick实现其功能
 */
class Component{
private:
    GObject* owner; // Weak reference
public:
    Component() : owner(nullptr) {}
    virtual ~Component() = default;
    GObject* get_owner() const noexcept {
        return owner;
    }
protected:
    friend class GObject;
    /**
     * @brief 当owner初始化时调用
     * 
     * owner在加入World后进行初始化，过程中调用每个组件的on_register，此时其他组建和物体都加载完成，但可能没有初始化
     */
    virtual void on_register(){
        assert(owner != nullptr);// 一个Component只能有一个owner
    }

    /**
     * @brief 当组件移出游戏的调用
     */
    virtual void on_unregister(){
        // 
        assert(owner != nullptr);
        // owner = nullptr 由GObject在detach后执行
    }

    virtual void on_update(ComponentUpdateFlag flag) {
        assert(get_owner() != nullptr); // 而且必然已注册
        assert(flag != ComponentUpdateFlag::NONE); // 屁事没有则不会更新
    }
private:    
    friend class GObject;
    void set_owner(GObject* owner){
        this->owner = owner;
    }
};
}