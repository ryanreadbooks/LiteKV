#include <iostream>
#include "../core.h"

using namespace std;

int main(int argc, char const *argv[]) {

  cout << "==================== Begin test core kvcontainer 2 ====================\n";

  KVContainer engine;
  int errcode = 0;
  for (int i = 0; i < 10; ++i) {
    engine.LeftPush("l1", to_string(i), errcode);
  }
  cout << "After push left, l1 list len = " << engine.ListLen("l1", errcode) << endl;

  for (int i = 0; i < 5; ++i) {
    cout << engine.RightPop("l1", errcode) << " ";
  }
  cout << "\nAfter pop right, l1 list len = " << engine.ListLen("l1", errcode) << endl;

  for (int i = 0; i < 6; ++i) {
    auto item = engine.LeftPop("l1", errcode);
    if (item.Empty()) {
      cout << "nil ";
    } else {
      cout << item << " ";
    }
  }
  cout << "\nAfter pop left, l1 list len = " << engine.ListLen("l1", errcode) << endl;

  cout << "==================== End test core kvcontainer   2 ====================\n";
  return 0;
}