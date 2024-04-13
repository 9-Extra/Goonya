#pragma once

#include "core/world/GObject.h"
#include <functional>

namespace Goonya {

class CpntTick : public Component {
public:
    using Callback = std::function<void(GObject &owner)>;
    static void default_callback(GObject &owner){/*do nothing*/};

    CpntTick(Callback &&on_tick, Callback &&on_attach = default_callback,
             Callback &&on_detach = default_callback)
        : on_tick(on_tick), on_attach(on_attach), on_detach(on_attach) {}

    virtual void attach() override {
        on_detach(*get_owner());
    }
    virtual void detach() override {
        on_detach(*get_owner());
    }
    virtual void tick() override {
        assert(get_owner() != nullptr);
        on_tick(*get_owner());
    }

private:
    Callback on_tick, on_attach, on_detach;
};
} // namespace Goonya
