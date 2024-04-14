#pragma once

#include <assert.h>

namespace Goonya {
class GObject;
// 组件基类，组件挂在GObject上，在每个tick时会被调用，不同的组件通过重载tick实现其功能
class Component{
public:
    Component() : owner(nullptr) {}
    GObject* get_owner() const{
        return owner;
    }
    virtual ~Component() = default;
protected:
    friend class GObject;
    virtual void attach(){
        // 设置owner由GObject执行
        assert(owner != nullptr);// 一个Component只能有一个owner
    }
    virtual void detach(){
        assert(owner != nullptr);
        // owner = nullptr 由GObject在detach后执行
    }
    virtual void tick() = 0;
private:    
    GObject* owner; // Weak reference
    friend class GObject;
    void set_owner(GObject* owner){
        this->owner = owner;
    }
};
}