#pragma once

#include "craft/level/level.h"
#include "function/world/World.h"
#include <function/world/GObject.h>

class MoveSystem final : public Goonya::Component, public Goonya::TickFunction {
public:
    void handle_mouse() const;
    void handle_keyboard(float delta) const;

    void on_register() override;
    void on_unregister() override;
    void tick() override;

private:
    std::shared_ptr<Goonya::GObject> cube;
    std::shared_ptr<Goonya::GObject> lights;
    std::shared_ptr<Goonya::GObject> light1;
    std::shared_ptr<Goonya::GObject> camera;
    std::shared_ptr<Goonya::GObject> teapot;

    Ref<Craft::Level> level;
};