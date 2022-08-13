#include <iostream>
#include "../src/hashset.h"

using namespace std;

/**
 * Test src/hashset.h(cpp)
 * @return
 */
int main() {

  auto displayStrings = [](const vector<DynamicString> &strings) {
    for (size_t i = 0; i < strings.size(); ++i) {
      if (i != strings.size() - 1) {
        cout << strings[i] << ", ";
      } else {
        cout << strings[i] << endl;
      }
    }
  };

  auto displayEntries = [] (const vector<HSEntry*>& entries) {
    for (size_t i = 0; i < entries.size(); ++i) {
      if (i != entries.size() - 1) {
        cout << *(entries[i]->key) << ", ";
      } else {
        cout << *(entries[i]->key) << endl;
      }
    }
  };

  HashSet set1;

  set1.Insert("root");
  set1.Insert("root");
  cout << set1.Count() << '\n';

  for (int i = 0; i < 26; ++i) {
    set1.Insert(to_string(i));
  }
  cout << set1.Count() << '\n';

  displayStrings(set1.AllKeys());
  displayEntries(set1.AllEntries());

  cout << boolalpha << set1.CheckExists("2323") << '\n';
  cout << boolalpha << set1.CheckExists("root") << '\n';
  cout << boolalpha << set1.CheckExists("15") << '\n';
  set1.Erase("root");
  cout << boolalpha << set1.CheckExists("root") << '\n';
  set1.Erase("3");
  cout << boolalpha << set1.CheckExists("3") << '\n';
  displayStrings(set1.AllKeys());
  cout << "set1.Count() = " << set1.Count() << '\n';

  return 0;
}