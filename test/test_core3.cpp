#include <iostream>
#include "../src/core.h"
#include "../src/mem.h"

using namespace std;

int main() {

  cout << "==================== Test container for hashtable ====================\n";

  auto showHashStr = [](const vector<DynamicString> &str) {
    if (str.empty()) {
      cout << "Empty\n";
      return;
    }
    for (size_t i = 0; i < str.size(); ++i) {
      if (i != str.size() - 1) {
        cout << str[i] << " ";
      } else {
        cout << str[i] << endl;
      }
    }
  };

  auto showHashTable = [&](const std::string &name,
                           const vector<DynamicString> &keys,
                           const vector<DynamicString> &values) {
    cout << "-------------------------\n";
    cout << name << " all keys:   ";
    showHashStr(keys);
    cout << name << " all values: ";
    showHashStr(values);
    cout << "-------------------------\n";
  };

  KVContainer engine;
  int errcode;
  engine.HashUpdateKV("ht1", "k1", "v1", errcode);
  auto v1 = engine.HashGetValue("ht1", "k1", errcode);
  if (errcode == kOkCode) {
    cout << v1 << endl;
  }

  int n = 100;
  srand(time(NULL));
  srandom(time(NULL));
  for (int i = 0; i < n; ++i) {
    engine.SetInt(to_string(rand()), rand());
  }

  for (int i = 0; i < n; ++i) {
    engine.HashUpdateKV(to_string(rand()), to_string(rand()), to_string(rand()), errcode);
  }

  engine.SetInt("int1", 5620);
  if (!engine.HashUpdateKV("int1", "k1", "5620k", errcode)) {
    cout << errcode << endl;
  }

  engine.LeftPush("l1", {"1", "2", "3", "4", "5"}, errcode);
  if (!engine.HashUpdateKV("l1", "k1", "5620k", errcode)) {
    cout << errcode << endl;
  }

  for (int i = 0; i < 5; i++) {
    engine.HashUpdateKV("kv1", to_string(i), to_string(i), errcode);
  }
  showHashTable("kv1",
                engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));
  cout << "kv1333 all values: ";
  showHashStr(engine.HashGetAllValues("kv1333", errcode));
  for (int i = 0; i < 5; i++) {
    int val = rand() % 20;
    string field = to_string(val);
    string value = to_string(val * 2);
    cout << "setting " << field << " to " << value << endl;
    engine.HashUpdateKV("kv1", field, value, errcode);
  }

  showHashTable("kv1",
                engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));

  cout << "adding field f1, f2 to kv1... \n";
  engine.HashUpdateKV("kv1", "f1", "v1", errcode);
  engine.HashUpdateKV("kv1", "f2", "v2", errcode);
  cout << "kv1 len = " << engine.HashLen("kv1", errcode);

  showHashTable("kv1",
                engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));

  cout << "deleting f1 from kv1...\n";
  engine.HashDelField("kv1", "f1", errcode);
  showHashTable("kv1",
                engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));
  cout << "kv1 len = " << engine.HashLen("kv1", errcode);

  cout << "checking exists k23-ew\n";
  cout << boolalpha << engine.HashExistField("k23", "ew", errcode) << endl;
  cout << "deleting a non-existing key kee-oko: "
       << engine.HashDelField("kee", "oko", errcode) << endl;
  cout << "deleting an existing key kv1-f2: "
       << engine.HashDelField("kv1", "f2", errcode) << endl;

  showHashStr(engine.HashGetAllEntries("kv1", errcode));

  cout << "\n--------------- Test multiple keys values operation ---------------\n";

  vector<string> fields {"name", "age", "ip", "address"};
  vector<string> values {"lily", "20", "127.0.0.1", "earth"};
  auto key = Key("person");
  engine.HashUpdateKV(key, fields, values, errcode);

  for (const auto& val : engine.HashGetValue(key, fields, errcode)) {
    if (val.Null()) {
      cout << "(nil), ";
    } else {
      cout << val << ", ";
    }
    cout << endl;
  }

  cout << "another round => \n";
  vector<string> fields2 {"nam1e", "age", "2ip", "address"};
  for (const auto& val : engine.HashGetValue(key, fields2, errcode)) {
    if (val.Null()) {
      cout << "(nil), ";
    } else {
      cout << val << ", ";
    }
    cout << endl;
  }

  showHashTable("person",
                engine.HashGetAllFields("person", errcode),
                engine.HashGetAllValues("person", errcode));

  cout << "--------------- Test multiple keys values operation end ---------------\n";

  cout << "Memory status: ";
  cout << ProcessVmSizeAsString() << endl;
  cout << "==================== Test container for hashtable End ====================\n";

  return 0;
}