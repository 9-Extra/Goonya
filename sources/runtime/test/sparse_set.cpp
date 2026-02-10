#include <gtest/gtest.h>
#include <unordered_set>

#include "core/sparse_set.h"

using namespace Goonya;

// 测试基本插入和包含检查
TEST(SparseSetTest, InsertAndContains) {
    SparseSet<std::string> set;

    // 插入元素
    auto handle1 = set.insert("Apple");
    auto handle2 = set.insert("Banana");
    auto handle3 = set.insert("Cherry");

    // 检查插入后包含这些元素
    EXPECT_TRUE(set.contains(handle1));
    EXPECT_TRUE(set.contains(handle2));
    EXPECT_TRUE(set.contains(handle3));

    // 检查获取的元素正确
    EXPECT_EQ(set[handle1], "Apple");
    EXPECT_EQ(set[handle2], "Banana");
    EXPECT_EQ(*set.get_or_null(handle3), "Cherry");

    // 检查大小
    EXPECT_EQ(set.size(), 3);
    EXPECT_FALSE(set.empty());
}

// 测试删除功能
TEST(SparseSetTest, Remove) {
    SparseSet<int> set;

    // 插入元素
    auto handle1 = set.emplace(10);
    auto handle2 = set.emplace(20);
    auto handle3 = set.emplace(30);
    auto handle4 = set.emplace(40);

    // 删除中间元素
    set.remove(handle2);

    // 检查删除后的状态
    EXPECT_FALSE(set.contains(handle2)); // 被删除的元素不应存在
    EXPECT_TRUE(set.contains(handle1));  // 其他元素应存在
    EXPECT_TRUE(set.contains(handle3));
    EXPECT_TRUE(set.contains(handle4));
    EXPECT_EQ(set.size(), 3);

    // 再次删除同一个句柄（应该没效果）
    set.remove(handle2);
    EXPECT_EQ(set.size(), 3); // 大小不应变化

    // 删除多个元素
    set.remove(handle1);
    set.remove(handle4);

    EXPECT_FALSE(set.contains(handle1));
    EXPECT_TRUE(set.contains(handle3)); // 只剩这个元素
    EXPECT_FALSE(set.contains(handle4));
    EXPECT_EQ(set.size(), 1);

    // 删除最后一个元素
    set.remove(handle3);
    EXPECT_TRUE(set.empty());
}

// 测试版本号机制
TEST(SparseSetTest, Versioning) {
    SparseSet<std::string> set;

    // 插入元素
    auto handle1 = set.insert("First");

    // 删除元素
    set.remove(handle1);
    EXPECT_FALSE(set.contains(handle1)); // 删除后应无效

    // 在相同位置插入新元素（如果实现重用位置）
    auto handle2 = set.insert("Second");

    // 检查版本号（如果实现正确，新句柄应有不同的版本号）
    // 注意：这取决于您的实现是否立即重用位置
    if (handle1.index == handle2.index) {
        EXPECT_NE(handle1.version, handle2.version); // 相同位置应有不同的版本号
    }

    // 旧句柄不应访问新元素
    EXPECT_FALSE(set.contains(handle1));
    EXPECT_TRUE(set.contains(handle2));
}

// 测试遍历功能
TEST(SparseSetTest, Iteration) {
    SparseSet<int> set;

    // 插入一些元素
    std::vector<int> values = {1, 2, 3, 4, 5};
    std::vector<Handle<int>> handles;
    handles.reserve(values.size());

    for (int value : values) {
        handles.push_back(set.insert(value));
    }

    // 删除部分元素
    set.remove(handles[1]); // 删除第二个元素（值2）
    set.remove(handles[3]); // 删除第四个元素（值4）

    // 测试范围for循环
    std::vector<int> collected;
    for (int value : set) {
        collected.push_back(value);
    }

    // 检查收集到的元素（应包含1, 3, 5，顺序可能不同）
    std::sort(collected.begin(), collected.end());
    std::vector<int> expected = {1, 3, 5};
    EXPECT_EQ(collected, expected);

    // 测试迭代器手动遍历
    collected.clear();
    for (auto it = set.begin(); it != set.end(); ++it) { // NOLINT
        collected.push_back(*it);
    }

    std::sort(collected.begin(), collected.end());
    EXPECT_EQ(collected, expected);

    // 测试const迭代
    const auto &const_set = set;
    collected.clear();
    for (int value : const_set) {
        collected.push_back(value);
    }

    std::sort(collected.begin(), collected.end());
    EXPECT_EQ(collected, expected);
}

// 测试大规模插入删除
TEST(SparseSetTest, LargeScaleOperations) {
    SparseSet<int> set;
    const int N = 1000;
    std::vector<Handle<int>> handles;
    handles.reserve(N);

    // 插入N个元素
    for (int i = 0; i < N; ++i) {
        handles.push_back(set.insert(i));
    }
    EXPECT_EQ(set.size(), N);

    // 验证所有元素都存在
    for (int i = 0; i < N; ++i) {
        EXPECT_TRUE(set.contains(handles[i]));
        EXPECT_EQ(set[handles[i]], i);
    }

    // 删除所有偶数索引的元素
    int removed_count = 0;
    for (int i = 0; i < N; i += 2) {
        set.remove(handles[i]);
        removed_count++;
    }
    EXPECT_EQ(set.size(), N - removed_count);

    // 验证删除的元素不存在，未删除的元素存在
    for (int i = 0; i < N; ++i) {
        if (i % 2 == 0) {
            EXPECT_FALSE(set.contains(handles[i]));
        } else {
            EXPECT_TRUE(set.contains(handles[i]));
            EXPECT_EQ(set[handles[i]], i);
        }
    }

    // 遍历剩余元素
    int sum = 0;
    for (int value : set) {
        sum += value;
    }

    // 计算期望的和（所有奇数的和）
    int expected_sum = 0;
    for (int i = 1; i < N; i += 2) {
        expected_sum += i;
    }
    EXPECT_EQ(sum, expected_sum);
}

// 测试插入删除后遍历的一致性
TEST(SparseSetTest, IterationConsistency) {
    SparseSet<std::string> set;

    // 插入元素
    auto h1 = set.insert("A"); // NOLINT
    auto h2 = set.insert("B");
    auto h3 = set.insert("C"); // NOLINT

    // 收集第一次遍历结果
    std::unordered_set<std::string> first_pass;
    for (const auto &s : set) {
        first_pass.insert(s);
    }

    // 应该包含所有三个元素
    EXPECT_EQ(first_pass.size(), 3);
    EXPECT_TRUE(first_pass.count("A"));
    EXPECT_TRUE(first_pass.count("B"));
    EXPECT_TRUE(first_pass.count("C"));

    // 删除一个元素
    set.remove(h2);

    // 收集第二次遍历结果
    std::unordered_set<std::string> second_pass;
    for (const auto &s : set) {
        second_pass.insert(s);
    }

    // 应该只包含两个元素
    EXPECT_EQ(second_pass.size(), 2);
    EXPECT_TRUE(second_pass.count("A"));
    EXPECT_FALSE(second_pass.count("B")); // B已被删除
    EXPECT_TRUE(second_pass.count("C"));

    // 再插入新元素
    auto h4 = set.insert("D"); // NOLINT

    // 收集第三次遍历结果
    std::unordered_set<std::string> third_pass;
    for (const auto &s : set) {
        third_pass.insert(s);
    }

    // 应该包含三个元素
    EXPECT_EQ(third_pass.size(), 3);
    EXPECT_TRUE(third_pass.count("A"));
    EXPECT_FALSE(third_pass.count("B"));
    EXPECT_TRUE(third_pass.count("C"));
    EXPECT_TRUE(third_pass.count("D"));
}

// 测试句柄持久性
TEST(SparseSetTest, HandlePersistence) {
    SparseSet<int> set;

    // 插入元素并保存句柄
    auto handle = set.insert(42);
    uint32_t saved_index = handle.index;
    uint32_t saved_version = handle.version;

    // 句柄应该有效
    EXPECT_TRUE(set.contains(handle));
    EXPECT_EQ(set[handle], 42);

    // 插入更多元素（可能触发重新分配）
    for (int i = 0; i < 100; ++i) {
        set.insert(i * 10);
    }

    // 原始句柄仍然应该有效
    EXPECT_TRUE(set.contains(handle));
    EXPECT_EQ(set[handle], 42);
    EXPECT_EQ(handle.index, saved_index);     // 索引应该不变
    EXPECT_EQ(handle.version, saved_version); // 版本应该不变

    // 删除元素
    set.remove(handle);
    EXPECT_FALSE(set.contains(handle));
}

// 测试空集合的迭代
TEST(SparseSetTest, EmptyIteration) {
    SparseSet<int> set;

    // 空集合的遍历应该不执行循环体
    int count = 0;
    for (int _ : set) {
        count++;
    }
    EXPECT_EQ(count, 0);

    // begin() 应该等于 end()
    EXPECT_EQ(set.begin(), set.end());
}

// 测试元素修改
TEST(SparseSetTest, ElementModification) {
    SparseSet<std::string> set;

    auto handle = set.insert("Initial");
    EXPECT_EQ(set[handle], "Initial");

    // 修改元素
    set[handle] = "Modified";
    EXPECT_EQ(set[handle], "Modified");

    // 通过迭代器修改
    for (auto &s : set) {
        s = "Changed";
    }
    EXPECT_EQ(set[handle], "Changed");
}

TEST(SparseSetTest, Emplace) {
    struct Data {
        int a;
        float b;
    };

    SparseSet<Data> set;

    auto handle = set.emplace(42, 3.14f);
    EXPECT_EQ(set[handle].a, 42);
    EXPECT_EQ(set[handle].b, 3.14f);
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}