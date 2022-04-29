#include "../src/dlist.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, char const *argv[]) {
  cout << "sizeof(ElemType) = " << sizeof(ElemType) << endl;
  cout << "sizeof(Node) = " << sizeof(Node) << endl;
  cout << "sizeof(DList) = " << sizeof(DList) << endl;

  DList dlst;

  // dlst.PushLeft("hello", 5);
  // dlst.PushLeft("okok", 4);
  // dlst.PushLeft("one", 3);
  // dlst.PushRight("world", 5);
  // dlst.PushRight("ab", 2);

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

  auto traverse = [&](DList &lst, bool detail = true) {
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

  cout << "===================== Test push function =====================\n";

  for (int i = 0; i < 20; i++) {
    dlst.PushLeft(to_string(i));
  }
  auto range = dlst.RangeAsStdStringVector();
  traverse(dlst);

  for (int i = 20; i < 50; i++) {
    dlst.PushRight(to_string(i));
  }

  range = dlst.RangeAsStdStringVector();
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
  cout << "dlst[0] = " << dlst[0] << endl;
  cout << "dlst[3] = " << dlst[3] << endl;
  cout << "dlst[4] = " << dlst[4] << endl;
  cout << "dlst[5] = " << dlst[5] << endl;
  cout << "dlst[1] = " << dlst[1] << endl;
  cout << "dlst[7] = " << dlst[7] << endl;
  cout << "dlst[16] = " << dlst[16] << endl;
  cout << "dlst[24] = " << dlst[24] << endl;
  for (int i = 0; i < 20; ++i) {
    dlst.PushLeft(to_string(i));
  }
  traverse(dlst);
  cout << "dlst[0] = " << dlst[0] << endl;
  cout << "dlst[2] = " << dlst[2] << endl;
  cout << "dlst[3] = " << dlst[3] << endl;
  cout << "dlst[12] = " << dlst[12] << endl;
  cout << "dlst[17] = " << dlst[17] << endl;
  cout << "dlst[21] = " << dlst[21] << endl;
  cout << "dlst[36] = " << dlst[36] << endl;
  cout << "dlst[42] = " << dlst[42] << endl;
  cout << "dlst[44] = " << dlst[44] << endl;

  cout
      << "\n===================== Random push pop test =====================\n";
  // random push and pop
  srand(time(NULL));
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

  return 0;
}
