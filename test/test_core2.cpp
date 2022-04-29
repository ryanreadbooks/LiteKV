#include <iostream>
#include "../src/core.h"

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

  cout << "--------------- Test key override ---------------\n";
  engine.SetInt("l1", -96);
  ValueObjectPtr val_ptr = engine.Get("l1", errcode);
  if (errcode == kOkCode) {
    cout << "l1 becomes int, val = " << val_ptr->ToInt64() << endl;
  }
  engine.SetString("l1", "hello world");
  val_ptr = engine.Get("l1", errcode);
  if (errcode == kOkCode) {
    cout << "l1 becomes string, val = " << val_ptr->ToStdString() << endl;
  }

  engine.LeftPush("l1", "5", errcode);
  if (errcode == kWrongTypeCode) {
    cout << "left push into l1 failed, wrong type\n";
  }

  auto dy = engine.RightPop("l1", errcode);
  if (errcode == kWrongTypeCode) {
    cout << "left right pop l1 failed, wrong type\n";
    cout << boolalpha << "dy.Empty() ? " << dy.Empty() << endl;
  }

  auto dy2 = engine.LeftPop("l1", errcode);
  if (errcode == kWrongTypeCode) {
    cout << "left right pop l1 failed, wrong type\n";
    cout << boolalpha << "dy2.Empty() ? " << dy2.Empty() << endl;
  }
  engine.RightPush("l1", "5", errcode);
  if (errcode == kWrongTypeCode) {
    cout << "right push into l1 failed, wrong type\n";
  }
  cout << "deleted key l1, success? " << boolalpha << engine.Delete("l1") << endl;

  val_ptr = engine.Get("l1", errcode);
  if (errcode == kKeyNotFoundCode) {
    cout << "key l1 not found\n";
  }

  for (int i = 0; i < 5; ++i) {
    engine.LeftPush("list2", to_string(i), errcode);
  }
  for (int i = 5; i < 10; ++i) {
    engine.RightPush("list2", to_string(i * 2), errcode);
  }
  auto show_range = [](const vector<ElemType> &ranges)
  {
    if (ranges.empty())
    {
      cout << "Empty\n";
      return;
    }
    for (auto&& item : ranges)
    {
      if (item.Empty())
      {
        cout << "nil ";
      }
      else
      {
        cout << item << " ";  
      }
    }
    cout << endl;
  };
  // 4 3 2 1 0 10 12 14 16 18
  cout << "list2 len = " << engine.ListLen("list2", errcode) << ",items are: ";
  auto list2 = engine.ListRange("list2", 0, 9, errcode);
  cout << "[0, 9] => ";
  show_range(list2);

  cout << "[0, -1] => ";
  list2 = engine.ListRange("list2", 0, -1, errcode);
  show_range(list2);

  cout << "[-5, -1] => ";
  list2 = engine.ListRange("list2", -5, -1, errcode);
  show_range(list2);

  cout << "[2, -4] => ";
  list2 = engine.ListRange("list2", 2, -4, errcode);
  show_range(list2);

  cout << "[-10, -11] => ";
  list2 = engine.ListRange("list2", -10, -11, errcode);
  show_range(list2);

  cout << "[8, -2] => ";
  list2 = engine.ListRange("list2", 8, -2, errcode);
  show_range(list2);

  cout << "[-16, 78] => ";
  list2 = engine.ListRange("list2", -16, 78, errcode);
  show_range(list2);
  cout << endl;

  cout << " ----------- Test list operator[] ----------- \n";
  
  list2 = engine.ListRange("list2", 0, -1, errcode);
  for (size_t i = 0; i < list2.size(); ++i) {
    string opval = engine.ListItemAtIndex("list2", i, errcode).ToStdString();
    if (opval.empty()) {
      cout << "list2[" << i << "] = nil\n";
    } else {
      cout << "list2[" << i << "] = " << opval << endl;
    }
  }
  for (int i = -1; i >= -15; --i) {
    string opval = engine.ListItemAtIndex("list2", i, errcode).ToStdString();
    if (opval.empty()) {
      cout << "list2[" << i << "] = nil\n";
    } else {
      cout << "list2[" << i << "] = " << opval << endl;
    }
  }
  cout << " -------------- list set item at index -------------- \n";
  string opval = engine.ListItemAtIndex("list2", 5, errcode).ToStdString();
  cout << "list2[5] = " << opval << endl;
  engine.ListSetItemAtIndex("list2", 5, "changed", errcode);
  cout << "list2[5] = " <<  engine.ListItemAtIndex("list2", 5, errcode) << endl;
  show_range(engine.ListRange("list2", 0, -1, errcode));

  cout << "deleted key list2 ? " << boolalpha << engine.Delete(Key("list2")) << endl;

  cout << "==================== End test core kvcontainer   2 ====================\n";
  return 0;
}