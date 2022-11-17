#include <gtest/gtest.h>
#include <iostream>
#include "../src/core.h"
#include "../src/mem.h"

using namespace std;
KVContainer engine;
int errcode;

auto show_range = [](const vector<ElemType> &ranges) {
  if (ranges.empty()) {
    cout << "Empty\n";
    return;
  }
  for (auto &&item : ranges) {
    if (item.Empty()) {
      cout << "nil ";
    } else {
      cout << item << " ";
    }
  }
  cout << endl;
};

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

auto showHashTable = [](const std::string &name, const vector<DynamicString> &keys,
                        const vector<DynamicString> &values) {
  cout << "-------------------------\n";
  cout << name << " all keys:   ";
  showHashStr(keys);
  cout << name << " all values: ";
  showHashStr(values);
  cout << "-------------------------\n";
};

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

TEST(KVContainerTest, TestInt) {
  cout << "sizeof(Key) = " << sizeof(Key) << endl;
  cout << "sizeof(ValueObject) = " << sizeof(ValueObject) << endl;
  engine.SetInt("int1", 15260);

  ValueObjectPtr p1 = engine.Get("int1", errcode);
  if (errcode == kOkCode) {
    EXPECT_EQ(reinterpret_cast<int64_t>(p1->ptr), 15260);
  }
  engine.SetInt("int2", -52);
  engine.SetInt("int3", 50);
  engine.SetInt("int1", 102);
  engine.SetInt("int4", 1529656622102);

  p1 = engine.Get("int1", errcode);
  EXPECT_EQ(reinterpret_cast<int64_t>(p1->ptr), 102);
  p1 = engine.Get("int2", errcode);
  EXPECT_EQ(reinterpret_cast<int64_t>(p1->ptr), -52);
  p1 = engine.Get("int3", errcode);
  EXPECT_EQ(reinterpret_cast<int64_t>(p1->ptr), 50);
  p1 = engine.Get("int4", errcode);
  EXPECT_EQ(reinterpret_cast<int64_t>(p1->ptr), 1529656622102);
  p1 = engine.Get("int4", errcode);
  EXPECT_EQ(reinterpret_cast<int64_t>(p1->ptr), 1529656622102);
};

TEST(KVContainerTest, TestString) {
  engine.SetString("str1", "hello");
  engine.SetString("str2", "world");
  engine.SetString("str3", "okok");
  engine.SetString("str4", "s11s12");

  ValueObjectPtr p2 = engine.Get("str1", errcode);
  if (p2 && errcode == kOkCode) {
    //    cout << p2->ToStdString() << endl;
    EXPECT_STREQ(p2->ToStdString().c_str(), "hello");
  }
  p2 = engine.Get("str2", errcode);
  if (p2 && errcode == kOkCode) {
    EXPECT_STREQ(p2->ToStdString().c_str(), "world");
  }
  p2 = engine.Get("str3", errcode);
  if (p2 && errcode == kOkCode) {
    EXPECT_STREQ(p2->ToStdString().c_str(), "okok");
  }
  p2 = engine.Get("str4", errcode);
  if (p2 && errcode == kOkCode) {
    EXPECT_STREQ(p2->ToStdString().c_str(), "s11s12");
  }

  engine.SetString("str4", "wonderful");
  p2 = engine.Get("str4", errcode);
  if (p2 && errcode == kOkCode) {
    EXPECT_STREQ(p2->ToStdString().c_str(), "wonderful");
  }
}

TEST(KVContainerTest, TestCover) {
  engine.SetInt("str4", 1100869);  // set existing key of different type
  auto p2 = engine.Get("str4", errcode);
  EXPECT_EQ(p2->ToInt64(), 1100869);

  engine.SetString("int4", "changing from int4!!!!");
  p2 = engine.Get("int4", errcode);
  EXPECT_STREQ(p2->ToStdString().c_str(), "changing from int4!!!!");
}

TEST(KVContainerTest, TestList) {
  for (int i = 0; i < 10; ++i) {
    engine.LeftPush("l1", to_string(i), errcode);
  }
  EXPECT_EQ(engine.ListLen("l1", errcode), 10);
  for (int i = 0; i < 5; ++i) {
    cout << engine.RightPop("l1", errcode) << " ";
  }
  EXPECT_EQ(engine.ListLen("l1", errcode), 5);
  for (int i = 0; i < 6; ++i) {
    auto item = engine.LeftPop("l1", errcode);
    if (item.Empty()) {
      cout << "nil ";
    } else {
      cout << item << " ";
    }
  }
  cout << "\nAfter pop left, l1 list len = " << engine.ListLen("l1", errcode) << endl;
  EXPECT_EQ(engine.ListLen("l1", errcode), 0);
  cout << "--------------- Test key override ---------------\n";
  engine.SetInt("l1", -96);
  ValueObjectPtr val_ptr = engine.Get("l1", errcode);
  if (errcode == kOkCode) {
    EXPECT_EQ(val_ptr->ToInt64(), -96);
  }
  engine.SetString("l1", "hello world");
  val_ptr = engine.Get("l1", errcode);
  if (errcode == kOkCode) {
    EXPECT_STREQ(val_ptr->ToStdString().c_str(), "hello world");
  }

  engine.LeftPush("l1", "5", errcode);
  EXPECT_EQ(errcode, kWrongTypeCode);

  auto dy = engine.RightPop("l1", errcode);
  EXPECT_EQ(errcode, kWrongTypeCode);
  EXPECT_TRUE(dy.Empty());

  auto dy2 = engine.LeftPop("l1", errcode);
  EXPECT_EQ(errcode, kWrongTypeCode);
  EXPECT_TRUE(dy.Empty());
  engine.RightPush("l1", "5", errcode);
  EXPECT_EQ(errcode, kWrongTypeCode);

  EXPECT_TRUE(engine.Delete("l1"));

  val_ptr = engine.Get("l1", errcode);
  EXPECT_EQ(errcode, kKeyNotFoundCode);

  for (int i = 0; i < 5; ++i) {
    engine.LeftPush("list2", to_string(i), errcode);
  }
  for (int i = 5; i < 10; ++i) {
    engine.RightPush("list2", to_string(i * 2), errcode);
  }

  // 4 3 2 1 0 10 12 14 16 18
  EXPECT_EQ(engine.ListLen("list2", errcode), 10);
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
  EXPECT_EQ(opval, "10");
  engine.ListSetItemAtIndex("list2", 5, "changed", errcode);
  EXPECT_EQ(engine.ListItemAtIndex("list2", 5, errcode), "changed");
  show_range(engine.ListRange("list2", 0, -1, errcode));

  EXPECT_TRUE(engine.Delete(Key("list2")));
}

TEST(KVContainerTest, TestHashDict) {
  engine.HashUpdateKV("ht1", "k1", "v1", errcode);
  auto v1 = engine.HashGetValue("ht1", "k1", errcode);
  EXPECT_EQ(errcode, kOkCode);
  EXPECT_STREQ(v1.ToStdString().c_str(), "v1");

  int n = 100;
  srand(666);
  srandom(666);
  for (int i = 0; i < n; ++i) {
    engine.SetInt(to_string(rand()), rand());
  }

  for (int i = 0; i < n; ++i) {
    engine.HashUpdateKV(to_string(rand()), to_string(rand()), to_string(rand()), errcode);
  }

  engine.SetInt("int1", 5620);
  if (!engine.HashUpdateKV("int1", "k1", "5620k", errcode)) {
    EXPECT_EQ(errcode, kWrongTypeCode);
  }

  engine.LeftPush("l1", {"1", "2", "3", "4", "5"}, errcode);
  if (!engine.HashUpdateKV("l1", "k1", "5620k", errcode)) {
    EXPECT_EQ(errcode, kWrongTypeCode);
  }

  for (int i = 0; i < 5; i++) {
    engine.HashUpdateKV("kv1", to_string(i), to_string(i), errcode);
  }
  showHashTable("kv1", engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));
  cout << "kv1333 all values: ";
  showHashStr(engine.HashGetAllValues("kv1333", errcode));
  EXPECT_TRUE(engine.HashGetAllValues("kv1333", errcode).empty());
  for (int i = 0; i < 5; i++) {
    int val = rand() % 20;
    string field = to_string(val);
    string value = to_string(val * 2);
    cout << "setting " << field << " to " << value << endl;
    engine.HashUpdateKV("kv1", field, value, errcode);
  }

  showHashTable("kv1", engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));

  cout << "adding field f1, f2 to kv1... \n";
  engine.HashUpdateKV("kv1", "f1", "v1", errcode);
  engine.HashUpdateKV("kv1", "f2", "v2", errcode);
  EXPECT_EQ(engine.HashLen("kv1", errcode), 10);

  showHashTable("kv1", engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));

  cout << "deleting f1 from kv1...\n";
  engine.HashDelField("kv1", "f1", errcode);
  showHashTable("kv1", engine.HashGetAllFields("kv1", errcode),
                engine.HashGetAllValues("kv1", errcode));
  EXPECT_EQ(engine.HashLen("kv1", errcode), 9);

  cout << "checking exists k23-ew\n";
  EXPECT_FALSE(engine.HashExistField("k23", "ew", errcode));
  //  cout << boolalpha << engine.HashExistField("k23", "ew", errcode) << endl;
  cout << "deleting a non-existing key kee-oko: " << engine.HashDelField("kee", "oko", errcode)
       << endl;
  EXPECT_EQ(engine.HashDelField("kee", "oko", errcode), 0);
  EXPECT_EQ(engine.HashDelField("kv1", "f2", errcode), 1);

  showHashStr(engine.HashGetAllEntries("kv1", errcode));

  cout << "\n--------------- Test multiple keys values operation ---------------\n";

  vector<string> fields{"name", "age", "ip", "address"};
  vector<string> values{"lily", "20", "127.0.0.1", "earth"};
  auto key = Key("person");
  engine.HashUpdateKV(key, fields, values, errcode);

  for (const auto &val : engine.HashGetValue(key, fields, errcode)) {
    if (val.Null()) {
      cout << "(nil), ";
    } else {
      cout << val << ", ";
    }
    cout << endl;
  }

  cout << "another round => \n";
  vector<string> fields2{"nam1e", "age", "2ip", "address"};
  int i = 0;
  for (const auto &val : engine.HashGetValue(key, fields2, errcode)) {
    if (val.Null()) {
      cout << "(nil), ";
    } else {
      cout << val << ", ";
    }
    cout << endl;
    switch (i) {
      case 0:
      case 2:
        EXPECT_TRUE(val.Null());
        break;
      case 1:
        EXPECT_EQ(val, "20");
        break;
      case 3:
        EXPECT_EQ(val, "earth");
        break;
    }
    i++;
  }

  showHashTable("person", engine.HashGetAllFields("person", errcode),
                engine.HashGetAllValues("person", errcode));

  cout << "--------------- Test multiple keys values operation end ---------------\n";

  cout << "Memory status: ";
  cout << ProcessVmSizeAsString() << endl;
  cout << "==================== Test container for hashtable End ====================\n";
}

TEST(KVContainerTest, TestHashSet) {
  EXPECT_TRUE(engine.SetAddItem("name", "john", errcode));
  EXPECT_TRUE(engine.SetAddItem("name", "mage", errcode));
  EXPECT_TRUE(engine.SetAddItem("name", "vision", errcode));
  EXPECT_TRUE(engine.SetAddItem("name", "world", errcode));
  EXPECT_TRUE(engine.SetAddItem("name", "hello", errcode));
  EXPECT_TRUE(engine.SetAddItem("name", "ok", errcode));

  EXPECT_TRUE(engine.SetIsMember("name", "john", errcode));
  EXPECT_FALSE(engine.SetIsMember("name", "mike", errcode));

  EXPECT_EQ(engine.SetGetMemberCount("name", errcode), 6);
  EXPECT_TRUE(engine.SetRemoveMembers("name", {"john"}, errcode));
  EXPECT_EQ(engine.SetGetMemberCount("name", errcode), 5);

  displayStrings(engine.SetGetMembers("name", errcode));

  engine.SetAddItem("weekdays", {"1", "2", "3", "4", "5"}, errcode);
  for (const auto &item : engine.SetGetMembers("weekdays", errcode)) {
    EXPECT_EQ(errcode, kOkCode);
    EXPECT_TRUE(item == "1" || item == "2" || item == "3" || item == "4" || item == "5");
  }
  displayStrings(engine.SetGetMembers("weekdays", errcode));

  displayStrings(engine.SetGetMembers("weekends", errcode));
  EXPECT_TRUE(engine.SetGetMembers("weekends", errcode).empty());

  stringstream ans;
  for (auto e : engine.SetMIsMember("weekdays", {"1", "2", "3", "roao", "4", "5", "sat", "world"},
                                    errcode)) {
    ans << e << ", ";
  }
  EXPECT_STREQ(ans.str().c_str(), "1, 1, 1, 0, 1, 1, 0, 0, ");

  for (auto e : engine.SetMIsMember("weekends", {"1", "2", "3", "4", "5"}, errcode)) {
    EXPECT_FALSE(e);
  }
  cout << '\n';
}

TEST(KVContainerTest, TestEmptyKeyName) {
  engine.SetInt("", 100);
  EXPECT_EQ(engine.Get("", errcode)->ToInt64(), 100);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
