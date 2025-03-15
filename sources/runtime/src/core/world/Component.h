#pragma once

#include <assert.h>

namespace Goonya {
class GObject;

/**
 * @brief 组件基类
 * 
 * 组件挂在GObject上，在每个tick时会被调用，不同的组件通过重载tick实现其功能
 */
class Component{
public:
    Component() : owner(nullptr) {}
    GObject* get_owner() const{
        return owner;
    }
    virtual ~Component() = default;
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
    /**
     * @brief 逻辑帧更新时调用
     */
    virtual void on_tick() = 0;
private:    
    GObject* owner; // Weak reference
    friend class GObject;
    void set_owner(GObject* owner){
        this->owner = owner;
    }
};
}