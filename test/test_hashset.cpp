#include <gtest/gtest.h>
#include <iostream>
#include "../src/hashset.h"

using namespace std;

auto displayStrings = [](const vector<DynamicString>& strings) {
  for (size_t i = 0; i < strings.size(); ++i) {
    if (i != strings.size() - 1) {
      cout << strings[i] << ", ";
    } else {
      cout << strings[i] << endl;
    }
  }
};

auto displayEntries = [](const vector<HSEntry*>& entries) {
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i != entries.size() - 1) {
      cout << *(entries[i]->key) << ", ";
    } else {
      cout << *(entries[i]->key) << endl;
    }
  }
};

TEST(HashSetTest, TestHashSet) {
  HashSet set1;

  set1.Insert("root");
  set1.Insert("root");
  EXPECT_EQ(1, set1.Count());

  for (int i = 0; i < 26; ++i) {
    set1.Insert(to_string(i));
  }
  EXPECT_EQ(27, set1.Count());

  displayStrings(set1.AllKeys());
  displayEntries(set1.AllEntries());

  EXPECT_FALSE(set1.CheckExists("2323"));
  EXPECT_TRUE(set1.CheckExists("root"));
  EXPECT_TRUE(set1.CheckExists("15"));
  set1.Erase("root");
  EXPECT_FALSE(set1.CheckExists("root"));
  set1.Erase("3");
  EXPECT_FALSE(set1.CheckExists("3"));
  displayStrings(set1.AllKeys());
  EXPECT_EQ(25, set1.Count());
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
