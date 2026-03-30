#include <gtest/gtest.h>
#include <memory>

#include "PrintTo.h"
#include "function/world/GObject.h"

using namespace Goonya;
TEST(Transform, intialize) {
    std::shared_ptr<GObject> obj = std::make_shared<GObject>();
    EXPECT_EQ(obj->get_local_position(), Vector3f(0, 0, 0));
    EXPECT_EQ(obj->get_local_rotation(), Quaternion::identity());
    EXPECT_EQ(obj->get_local_scale(), Vector3f(1, 1, 1));

    EXPECT_EQ(obj->get_global_position(), Vector3f(0, 0, 0));
    EXPECT_EQ(obj->get_global_rotation(), Quaternion::identity());
    EXPECT_EQ(obj->get_global_scale(), Vector3f(1, 1, 1));
}

TEST(Transform, concat) {
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

TEST(GObject, get_child_by_path) {
    // 构建一棵复杂的 GObject 树
    // root
    // ├── body
    // │   ├── head
    // │   │   └── eye
    // │   └── arm
    // └── wheel
    //     └── tire
    std::shared_ptr<GObject> root = std::make_shared<GObject>("root");
    std::shared_ptr<GObject> body = std::make_shared<GObject>("body");
    std::shared_ptr<GObject> head = std::make_shared<GObject>("head");
    std::shared_ptr<GObject> eye = std::make_shared<GObject>("eye");
    std::shared_ptr<GObject> arm = std::make_shared<GObject>("arm");
    std::shared_ptr<GObject> wheel = std::make_shared<GObject>("wheel");
    std::shared_ptr<GObject> tire = std::make_shared<GObject>("tire");

    root->attach_child(body);
    root->attach_child(wheel);
    body->attach_child(head);
    body->attach_child(arm);
    head->attach_child(eye);
    wheel->attach_child(tire);

    // 测试空路径返回自身
    EXPECT_EQ(root->get_child_by_path(""), root);
    EXPECT_EQ(body->get_child_by_path(""), body);
    EXPECT_EQ(eye->get_child_by_path(""), eye);

    // 测试查找（从当前节点开始）
    EXPECT_EQ(root->get_child_by_path("body"), body);
    EXPECT_EQ(root->get_child_by_path("wheel"), wheel);
    EXPECT_EQ(body->get_child_by_path("head"), head);
    EXPECT_EQ(body->get_child_by_path("arm"), arm);
    EXPECT_EQ(head->get_child_by_path("eye"), eye);
    EXPECT_EQ(wheel->get_child_by_path("tire"), tire);

    // 测试多级路径
    EXPECT_EQ(root->get_child_by_path("body/head"), head);
    EXPECT_EQ(root->get_child_by_path("body/head/eye"), eye);
    EXPECT_EQ(root->get_child_by_path("wheel/tire"), tire);
    EXPECT_EQ(body->get_child_by_path("head/eye"), eye);

    // 测试找不到返回 nullptr
    EXPECT_EQ(root->get_child_by_path("nonexistent"), nullptr);
    EXPECT_EQ(root->get_child_by_path("body/nonexistent"), nullptr);
    EXPECT_EQ(root->get_child_by_path("body/head/nonexistent"), nullptr);
    EXPECT_EQ(body->get_child_by_path("wheel"), nullptr); // wheel 不是 body 的子节点
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}