#pragma once

#include <core/cgmath.h>
#include <core/world/GObject.h>
#include <core/input/input.h>
#include <core/eventbus/eventbus.h>
#include <core/events.h>
#include <core/timer/timer.h>

class MoveSystem : public Goonya::Component {
public:
    void handle_mouse();
    void handle_keyboard(float delta);

    virtual void on_register() override;
    virtual void on_tick() override;

private:
    std::shared_ptr<Goonya::GObject> cube;
    std::shared_ptr<Goonya::GObject> lights;
    std::shared_ptr<Goonya::GObject> light1;
    std::shared_ptr<Goonya::GObject> camera;
    std::shared_ptr<Goonya::GObject> teapot;
};