#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include "../src/skiplist.h"

std::mt19937_64 sRandEngine(std::time(nullptr));

int rand_int(int64_t from, int64_t to) {
  return std::uniform_int_distribution<int64_t>(from, to)(sRandEngine);
}

TEST(SkiplistTest, BasicTest) {
  Skiplist sklist;
  EXPECT_TRUE(sklist.Empty());
  EXPECT_EQ(sklist.Count(), 0);
  EXPECT_EQ(sklist.Begin(), nullptr);
  EXPECT_EQ(sklist.End(), nullptr);
}

TEST(SkiplistTest, InsertMethodTest) {
  Skiplist sklist;
  EXPECT_TRUE(sklist.Insert(1, DynamicString("lily")));
  EXPECT_TRUE(sklist.Insert(100, DynamicString("elen")));
  EXPECT_TRUE(sklist.Insert(10, DynamicString("mika")));
  EXPECT_TRUE(sklist.Insert(20, DynamicString("riqo")));
  EXPECT_TRUE(sklist.Insert(50, DynamicString("koko")));
  EXPECT_TRUE(sklist.Insert(50, DynamicString("kinko")));
  EXPECT_FALSE(sklist.Insert(10, DynamicString("mika")));
  EXPECT_EQ(sklist.Count(), 6);
  EXPECT_FALSE(sklist.Empty());
  std::cout << sklist << std::endl;

  Skiplist* p_sklist2 = new Skiplist;
  for (int i = 0; i < 1000; ++i) {
    EXPECT_TRUE(p_sklist2->Insert(i, DynamicString(std::to_string(i))));
  }
  EXPECT_EQ(1000, p_sklist2->Count());
  delete p_sklist2;
}

TEST(SkiplistTest, ContainsMethodTest) {
  Skiplist sklist;
  sklist.Insert(10, DynamicString("liky"));
  EXPECT_FALSE(sklist.Insert(10, DynamicString("liky")));
  EXPECT_TRUE(sklist.Insert(60, DynamicString("liky")));
  EXPECT_TRUE(sklist.Insert(60, DynamicString("litzy")));
  std::cout << sklist << std::endl;
  // [10,liky]->[60,liky]->[60,litzy]
  EXPECT_TRUE(sklist.Contains(10, DynamicString("liky")));
  EXPECT_FALSE(sklist.Contains(10, DynamicString("1liky")));
  EXPECT_FALSE(sklist.Contains(100, DynamicString("1liky")));
  bool ans = sklist.Contains(60, DynamicString("liky"));
  EXPECT_TRUE(ans);
  EXPECT_TRUE(sklist.Contains(60, DynamicString("litzy")));
  EXPECT_FALSE(sklist.Contains(96, DynamicString("hello world")));
  EXPECT_EQ(sklist.Count(), 3);
  std::cout << sklist << std::endl;

  Skiplist list2;
  for (int i = 0; i < 10; ++i) {
    auto v = DynamicString(std::to_string(i));
    list2.Insert(i, v);
  }
  for (int i = 0; i < 10; ++i) {
    auto v = DynamicString(std::to_string(i));
    EXPECT_TRUE(list2.Contains(i, v));
  }
  for (int i = 11; i < 23; ++i) {
    auto v = DynamicString(std::to_string(i));
    EXPECT_FALSE(list2.Contains(i, v));
  }
  EXPECT_STREQ(list2.Begin()->data.ToStdString().c_str(), "0");
  EXPECT_EQ(list2.Begin()->score, 0);
  EXPECT_STREQ(list2.End()->data.ToStdString().c_str(), "9");
  EXPECT_EQ(list2.End()->score, 9);
}

TEST(SkiplistTest, RemoveMethodTest) {
  Skiplist sklist;
  EXPECT_TRUE(sklist.Insert(100, DynamicString("hello")));
  EXPECT_TRUE(sklist.Insert(10, DynamicString("world")));
  EXPECT_TRUE(sklist.Insert(5, DynamicString("ok")));
  EXPECT_TRUE(sklist.Insert(20, DynamicString("testing")));
  EXPECT_TRUE(sklist.Insert(50, DynamicString("working")));

  EXPECT_EQ(sklist.Count(), 5);

  /* try to remove some elements */
  EXPECT_TRUE(sklist.Remove(100, DynamicString("hello")));
  EXPECT_TRUE(sklist.Remove(10, DynamicString("world")));
  EXPECT_TRUE(sklist.Remove(5, DynamicString("ok")));
  EXPECT_TRUE(sklist.Remove(20, DynamicString("testing")));
  EXPECT_TRUE(sklist.Remove(50, DynamicString("working")));
  EXPECT_EQ(sklist.Count(), 0);

  EXPECT_TRUE(sklist.Insert(100, DynamicString("hello")));
  EXPECT_TRUE(sklist.Insert(10, DynamicString("world")));
  EXPECT_TRUE(sklist.Insert(5, DynamicString("ok")));
  EXPECT_TRUE(sklist.Insert(20, DynamicString("testing")));
  EXPECT_TRUE(sklist.Insert(50, DynamicString("working")));
  EXPECT_EQ(sklist.Count(), 5);
  std::cout << sklist << std::endl;

  Skiplist* p_sklist2 = new Skiplist;
  for (int i = 0; i < 100; ++i) {
    auto v = DynamicString(std::to_string(i));
    EXPECT_TRUE(p_sklist2->Insert(i, v));
    EXPECT_FALSE(p_sklist2->Insert(i, v));
    EXPECT_TRUE(p_sklist2->Remove(i, v));
    EXPECT_FALSE(p_sklist2->Remove(i, v));
  }
  EXPECT_TRUE(p_sklist2->Empty());
  EXPECT_EQ(p_sklist2->Count(), 0);
  delete p_sklist2;

  Skiplist list2;
  for (int i = 10; i < 20; ++i) {
    auto v = DynamicString(std::to_string(i));
    list2.Insert(i, v);
  }
  for (int i = 10; i < 20; ++i) {
    auto v = DynamicString(std::to_string(i));
    EXPECT_TRUE(list2.Remove(i, v));
  }
  for (int i = 0; i < 10; ++i) {
    auto v = DynamicString(std::to_string(i));
    EXPECT_FALSE(list2.Remove(i, v));
  }
  for (int i = 10; i < 20; ++i) {
    auto v = DynamicString(std::to_string(i));
    list2.Insert(i, v);
  }
  for (int i = 19; i >= 10; --i) {
    auto v = DynamicString(std::to_string(i));
    EXPECT_TRUE(list2.Remove(i, v));
  }
  EXPECT_EQ(list2.Count(), 0);
  EXPECT_EQ(list2.CurMaxLevel(), 0);
}

TEST(SkiplistTest, ClearMethodTest) {
  Skiplist l1;
  l1.Clear();

  EXPECT_TRUE(l1.Insert(100, DynamicString("hello")));
  EXPECT_TRUE(l1.Insert(100, DynamicString("hello world")));
  std::cout << l1 << std::endl;
  EXPECT_EQ(2, l1.Count());
  l1.Clear();
  EXPECT_EQ(0, l1.Count());
  EXPECT_TRUE(l1.Insert(100, DynamicString("hello")));
  EXPECT_TRUE(l1.Insert(100, DynamicString("hello world")));
  EXPECT_EQ(2, l1.Count());
}

TEST(SkiplistTest, MixedOperationTest) {
  Skiplist l1;
  EXPECT_TRUE(l1.Insert(100, DynamicString("hello")));
  EXPECT_TRUE(l1.Contains(100, DynamicString("hello")));
  for (int i = 5; i >= 0; i--) {
    EXPECT_TRUE(l1.Insert(i, DynamicString(std::to_string(i))));
  }
  EXPECT_EQ(7, l1.Count());
  EXPECT_TRUE(l1.Remove(4, DynamicString("4")));
  EXPECT_EQ(6, l1.Count());
  EXPECT_FALSE(l1.Contains(4, DynamicString("4")));
  std::cout << l1 << std::endl;
  EXPECT_EQ(l1.Begin()->score, 0);
  EXPECT_EQ(l1.End()->score, 100);
  l1.Clear();
  EXPECT_EQ(l1.Count(), 0);
  for (int i = 0; i < 20; ++i) {
    int tmp = rand_int(100, 1000000);
    EXPECT_FALSE(l1.Remove(tmp, DynamicString(std::to_string(tmp))));
  }
}

TEST(SkiplistTest, SkipOperatorBracketTest) {
  Skiplist sk1;

  sk1.Insert(100, DynamicString("rier"));
  sk1.Insert(200, DynamicString("fuel"));
  sk1.Insert(500, DynamicString("rice"));
  std::cout << sk1 << std::endl;
  sk1.Insert(300, DynamicString("wonder"));
  // TODO not finish yet
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}