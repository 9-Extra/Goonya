#include <gtest/gtest.h>
#include <memory>

#include "function/world/GObject.h"
#include "core/cgmath.h"
#include "PrintTo.h"

using namespace Goonya;
TEST(Transform, intialize){
    std::shared_ptr<GObject> obj = std::make_shared<GObject>();
    EXPECT_EQ(obj->get_local_position(), Vector3f(0, 0, 0));
    EXPECT_EQ(obj->get_local_rotation(), Quaternion::identity());
    EXPECT_EQ(obj->get_local_scale(), Vector3f(1, 1, 1));

    EXPECT_EQ(obj->get_global_position(), Vector3f(0, 0, 0));
    EXPECT_EQ(obj->get_global_rotation(), Quaternion::identity());
    EXPECT_EQ(obj->get_global_scale(), Vector3f(1, 1, 1));
}

TEST(Transform, concat){
    std::shared_ptr<GObject> parent = std::make_shared<GObject>("parent");
    std::shared_ptr<GObject> child = std::make_shared<GObject>("child");
    parent->attach_child(child);

    child->set_local_position({1, 0, 0});
    parent->set_local_rotation(Quaternion::from_rotation({0, 1, 0}, to_radian(90)));
    EXPECT_EQ(child->get_global_position(), Vector3f(0, 0, -1));

    parent->set_global_position({0, 1, 0});
    EXPECT_EQ(child->get_global_position(), Vector3f(0, 1, -1));

    child->set_global_position({3, 4, 5});
    EXPECT_EQ(child->get_global_position(), Vector3f(3, 4, 5));
    EXPECT_EQ(child->get_local_position(), Vector3f(-5, 3, 3));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}