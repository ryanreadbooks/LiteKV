#include "../src/dlist.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, char const *argv[]) {

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

  DList list;
  // for (int i = 0; i < 20; ++i) {
  //   list.PushLeft(to_string(i));
  // }
  for (int i = 0; i < 25; ++i) {
    list.PushRight(to_string(i));
  }
  traverse(list);

  auto range = list.RangeAsStdStringVector(1, 11);
  cout << "[1, 11]= ";
  showrange_std(range);

  cout << "[1, 3]= ";
  range = list.RangeAsStdStringVector(1, 3);
  showrange_std(range);

  cout << "[0, 9]= ";
  range = list.RangeAsStdStringVector(0, 9);
  showrange_std(range);

  cout << "[9, 10]= ";
  range = list.RangeAsStdStringVector(9, 10);
  showrange_std(range);

  cout << "[9, 11]= ";
  range = list.RangeAsStdStringVector(9, 11);
  showrange_std(range);

  cout << "[10, 11]= ";
  range = list.RangeAsStdStringVector(10, 11);
  showrange_std(range);

  cout << "[10, 14]= ";
  range = list.RangeAsStdStringVector(10, 14);
  showrange_std(range);

  cout << "[13, 14]= ";
  range = list.RangeAsStdStringVector(13, 14);
  showrange_std(range);

  cout << "[0, 14]= ";
  range = list.RangeAsStdStringVector(0, 14);
  showrange_std(range);

  cout << "[0, 20]= ";
  range = list.RangeAsStdStringVector(0, 20);
  showrange_std(range);
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

  return 0;
}