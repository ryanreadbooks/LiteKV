#include <iostream>
#include "../src/core.h"
#include "../src/mem.h"

using namespace std;

// test hashset functionality
int main() {

  auto displayStrings = [](const vector<DynamicString> &strings) {
    if (strings.empty()) {
      cout << "string is empty\n";
    }
    for (size_t i = 0; i < strings.size(); ++i) {
      if (i != strings.size() - 1) {
        cout << strings[i] << ", ";
      } else {
        cout << strings[i] << endl;
      }
    }
  };

  KVContainer engine;
  int errcode;

  cout << boolalpha << engine.SetAddItem("name", "john", errcode) << '\n';
  cout << boolalpha << engine.SetAddItem("name", "mage", errcode) << '\n';
  cout << boolalpha << engine.SetAddItem("name", "vision", errcode) << '\n';
  cout << boolalpha << engine.SetAddItem("name", "world", errcode) << '\n';
  cout << boolalpha << engine.SetAddItem("name", "hello", errcode) << '\n';
  cout << boolalpha << engine.SetAddItem("name", "ok", errcode) << '\n';

  cout << engine.SetIsMember("name", "john", errcode) << '\n';
  cout << engine.SetIsMember("name", "mike", errcode) << '\n';

  cout << engine.SetGetMemberCount("name", errcode) << '\n';
  cout << engine.SetRemoveMembers("name", {"john"}, errcode) << '\n';
  cout << engine.SetGetMemberCount("name", errcode) << '\n';

  displayStrings(engine.SetGetMembers("name", errcode));

  engine.SetAddItem("weekdays", {"1", "2", "3", "4", "5"}, errcode);
  displayStrings(engine.SetGetMembers("weekdays", errcode));

  displayStrings(engine.SetGetMembers("weekends", errcode));

  for (auto e : engine.SetMIsMember("weekdays", {"1", "2", "3", "roao", "4", "5", "sat", "world"}, errcode)) {
    cout << e << ", ";
  }

  for (auto e : engine.SetMIsMember("weekends", {"1", "2", "3", "4", "5"}, errcode)) {
    cout << e << ", ";
  }

  return 0;
}