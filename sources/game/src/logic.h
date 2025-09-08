#pragma once

#include "craft/level/level.h"
#include <core/world/GObject.h>

class MoveSystem : public Goonya::Component {
public:
    void handle_mouse() const;
    void handle_keyboard(float delta) const;

    void on_register() override;
    void on_tick() override;

private:
    std::shared_ptr<Goonya::GObject> cube;
    std::shared_ptr<Goonya::GObject> lights;
    std::shared_ptr<Goonya::GObject> light1;
    std::shared_ptr<Goonya::GObject> camera;
    std::shared_ptr<Goonya::GObject> teapot;

    Craft::Level level;
};