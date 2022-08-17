#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>
#include "../src/dlist.h"

using namespace std;
// check occupation distribution
auto check_distribution = [](DList &lst) {
  cout << "Node distribution is: ";
  Node *head = lst.Front();
  while (head) {
    cout << head->occupied << " ";
    head = head->next;
  }
  cout << endl;
};

auto showrange = [](const vector<DynamicString> &range) {
  cout << "Range size = " << range.size() << " items are : ";
  for (auto &&e : range) {
    if (e.Empty()) {
      cout << "nil ";
    } else {
      cout << e << " ";
    }
  }
  cout << endl;
};

auto showrange_std = [](const vector<string> &range) {
  cout << "Range size = " << range.size() << " items are : ";
  for (auto &&e : range) {
    if (e.empty()) {
      cout << "nil ";
    } else {
      cout << e << " ";
    }
  }
  cout << endl;
};

auto traverse = [](DList &lst, bool detail = true) {
  cout << "++++++ Current DList status is below +++++++\n";
  if (detail) {
    auto range = lst.RangeAsStdStringVector();
    cout << "range size = " << range.size() << endl;
    cout << "lst size = " << lst.Length() << endl;
    for (const auto &str : range) {
      cout << str << " ";
    }
  }
  cout << "\nNumber of nodes = " << lst.NodeNum() << endl;
  check_distribution(lst);
  cout << "+++++++++++++++++++++++++++\n";
};

TEST(DListTest, BasicTest) {
  cout << "sizeof(ElemType) = " << sizeof(ElemType) << endl;
  cout << "sizeof(Node) = " << sizeof(Node) << endl;
  cout << "sizeof(DList) = " << sizeof(DList) << endl;

  DList dlst;

  cout << "===================== Test push function =====================\n";

  for (int i = 0; i < 20; i++) {
    dlst.PushLeft(to_string(i));
  }
  auto range = dlst.RangeAsStdStringVector();
  vector<string> a = {"19", "18", "17", "16", "15", "14", "13", "12", "11", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1", "0"};
  EXPECT_TRUE(range == a);
  EXPECT_EQ(range.size(), 20);
  traverse(dlst);

  for (int i = 20; i < 50; i++) {
    dlst.PushRight(to_string(i));
  }
  range = dlst.RangeAsStdStringVector();
  EXPECT_EQ(range.size(), 50);
  traverse(dlst);

  // Test pop function
  cout << "===================== Test pop function =====================\n";
  vector<DynamicString> pops;
  for (int i = 0; i < 56; ++i) {
    DynamicString tmp = dlst.PopLeft();
    if (tmp.Length() != 0) {
      pops.emplace_back(tmp);
    }
  }
  cout << "After popleft: \n";
  range = dlst.RangeAsStdStringVector();
  EXPECT_EQ(range.size(), 0);
  traverse(dlst);

  cout << "This is the element popped: \n";
  for (auto &elem : pops) {
    cout << elem << " ";
  }

  cout << "\n===================== DList random access test "
          "=====================\n";
  for (int i = 0; i < 25; ++i) {
    dlst.PushRight(to_string(i));
  }
  traverse(dlst);
  EXPECT_EQ(dlst[0], "0");
  EXPECT_EQ(dlst[3], "3");
  EXPECT_EQ(dlst[4], "4");
  EXPECT_EQ(dlst[5], "5");
  EXPECT_EQ(dlst[1], "1");
  EXPECT_EQ(dlst[7], "7");
  EXPECT_EQ(dlst[16], "16");
  EXPECT_EQ(dlst[24], "24");

  for (int i = 0; i < 20; ++i) {
    dlst.PushLeft(to_string(i));
  }
  traverse(dlst);
  EXPECT_EQ(dlst[0], "19");
  EXPECT_EQ(dlst[2], "17");
  EXPECT_EQ(dlst[3], "16");
  EXPECT_EQ(dlst[12], "7");
  EXPECT_EQ(dlst[17], "2");
  EXPECT_EQ(dlst[21], "1");
  EXPECT_EQ(dlst[36], "16");
  EXPECT_EQ(dlst[42], "22");
  EXPECT_EQ(dlst[44], "24");

  cout << "\n===================== Random push pop test =====================\n";
  // random push and pop
  srand(time(nullptr));
  for (int i = 0; i < 100000; ++i) {
    double option = (double)rand() / RAND_MAX;
    if (option >= 0.0 && option < 0.25) {
      dlst.PushLeft(to_string(option));
    } else if (option >= 0.25 && option < 0.5) {
      dlst.PushRight(to_string(option));
    } else if (option >= 0.5 && option < 0.75) {
      dlst.PopLeft();
    } else {
      dlst.PopRight();
    }
  }
  traverse(dlst);

  cout << " -------- FreeRedundantNodes --------\n";
  dlst.FreeRedundantNodes();
  traverse(dlst, false);

  for (int i = 0; i < 10; ++i) {
    dlst.PushRight(to_string(i));
  }

  for (int i = 0; i < 10; ++i) {
    dlst.PushLeft(to_string(i));
  }
  traverse(dlst, false);

  cout << "\n===================== Test get range =====================\n";

  DList dlist2;

  for (int i = 0; i < 10; i++) {
    dlist2.PushLeft(to_string(i));
  }
  traverse(dlist2);

  for (int i = 0; i < 13; i++) {
    auto range2 = dlist2.RangeAsDynaStringVector(0, i);
    showrange(range2);
  }

  for (int i = 0; i < 13; i++) {
    auto range2 = dlist2.RangeAsStdStringVector(0, i);
    showrange_std(range2);
  }

  DList list2;

  for (int i = 0; i < 5; ++i) {
    list2.PushLeft(to_string(i));
  }
  for (int i = 5; i < 10; ++i) {
    list2.PushRight(to_string(i * 2));
  }

  showrange(list2.RangeAsDynaStringVector(0, 9));
  showrange_std(list2.RangeAsStdStringVector(0, 9));

  cout << "===================== End of test =====================\n";
}

TEST(DListTest, BasicTest2) {
  DList list;
  for (int i = 0; i < 25; ++i) {
    list.PushRight(to_string(i));
  }
  traverse(list);

  auto range = list.RangeAsStdStringVector(1, 11);
  vector<string> a = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
  EXPECT_TRUE(range == a);
  cout << "[1, 11]= ";
  showrange_std(range);

  cout << "[1, 3]= ";
  range = list.RangeAsStdStringVector(1, 3);
  showrange_std(range);
  a = {"1", "2", "3"};
  EXPECT_TRUE(range == a);

  cout << "[0, 9]= ";
  range = list.RangeAsStdStringVector(0, 9);
  showrange_std(range);
  a = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
  EXPECT_TRUE(range == a);

  cout << "[9, 10]= ";
  range = list.RangeAsStdStringVector(9, 10);
  showrange_std(range);
  a = {"9", "10"};
  EXPECT_TRUE(range == a);

  cout << "[9, 11]= ";
  range = list.RangeAsStdStringVector(9, 11);
  showrange_std(range);
  a = {"9", "10", "11"};
  EXPECT_TRUE(range == a);

  cout << "[10, 11]= ";
  range = list.RangeAsStdStringVector(10, 11);
  showrange_std(range);
  a = {"10", "11"};
  EXPECT_TRUE(range == a);

  cout << "[10, 14]= ";
  range = list.RangeAsStdStringVector(10, 14);
  showrange_std(range);
  a = {"10", "11", "12", "13", "14"};
  EXPECT_TRUE(range == a);

  cout << "[13, 14]= ";
  range = list.RangeAsStdStringVector(13, 14);
  showrange_std(range);
  a = {"13", "14"};
  EXPECT_TRUE(range == a);

  cout << "[0, 14]= ";
  range = list.RangeAsStdStringVector(0, 14);
  showrange_std(range);
  a = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14"};
  EXPECT_TRUE(range == a);

  cout << "[0, 20]= ";
  range = list.RangeAsStdStringVector(0, 20);
  showrange_std(range);
  a = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17","18", "19", "20"};
  EXPECT_TRUE(range == a);
  // ----------------------------------------------------
  auto range2 = list.RangeAsDynaStringVector(1, 11);
  cout << "[1, 11]= ";
  showrange(range2);

  cout << "[1, 3]= ";
  range2 = list.RangeAsDynaStringVector(1, 3);
  showrange(range2);

  cout << "[0, 9]= ";
  range2 = list.RangeAsDynaStringVector(0, 9);
  showrange(range2);

  cout << "[9, 10]= ";
  range2 = list.RangeAsDynaStringVector(9, 10);
  showrange(range2);

  cout << "[9, 11]= ";
  range2 = list.RangeAsDynaStringVector(9, 11);
  showrange(range2);

  cout << "[10, 11]= ";
  range2 = list.RangeAsDynaStringVector(10, 11);
  showrange(range2);

  cout << "[10, 14]= ";
  range2 = list.RangeAsDynaStringVector(10, 14);
  showrange(range2);

  cout << "[13, 14]= ";
  range2 = list.RangeAsDynaStringVector(13, 14);
  showrange(range2);

  cout << "[0, 14]= ";
  range2 = list.RangeAsDynaStringVector(0, 14);
  showrange(range2);

  cout << "[0, 20]= ";
  range2 = list.RangeAsDynaStringVector(0, 20);
  showrange(range2);
}


int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
