#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include "../src/skiplist.h"

TEST(SkiplistTest, NodeBasicTest) { 
  SkiplistNode<int> node(1);
  SkiplistNode<int> node2(2);
  SkiplistNode<double> node3(3.3, 5);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}