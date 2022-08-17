#include <gtest/gtest.h>
#include <iostream>
#include <unordered_map>
#include "../src/hashdict.h"

using namespace std;
auto displayStrings = [](const vector<DynamicString> &strings) {
  for (size_t i = 0; i < strings.size(); ++i) {
    if (i != strings.size() - 1) {
      cout << strings[i] << ", ";
    } else {
      cout << strings[i] << endl;
    }
  }
};

auto displayHashTable = [](const HashTable &ht) {
  cout << "-----------------------\n";
  cout << "HashTable count = " << ht.Count() << ", load factor = " << ht.LoadFactor() << endl;
  cout << "+ All keys are:   ";
  displayStrings(ht.AllKeys());
  cout << "+ All values are: ";
  displayStrings(ht.AllValues());
  cout << "-----------------------\n";
};

auto displayDict = [](const HashDict &d) {
  cout << "-----------------------\n";
  cout << "HashDict item count = " << d.Count() << endl;
  cout << "+ All keys are:   ";
  displayStrings(d.AllKeys());
  cout << "+ All values are: ";
  displayStrings(d.AllValues());
  cout << "-----------------------\n";
};

TEST(HashDictTest, TestHashTable) {
  cout << "sizeof(HTEntry) = " << sizeof(HTEntry) << endl;
  cout << "sizeof(HashTable) = " << sizeof(HashTable) << endl;
  cout << "sizeof(unordered_map) = " << sizeof(unordered_map<int, int>) << endl;
  cout << "sizeof(DynamicString) = " << sizeof(DynamicString) << endl;

  cout << "============== Test HashTable ==============\n";

  HashTable hashtable;
  hashtable.UpdateKV(HEntryKey("k1", 2), HEntryVal("v1", 2));
  hashtable.UpdateKV(HEntryKey("k2", 2), HEntryVal("v2", 2));
  hashtable.UpdateKV(HEntryKey("k3", 2), HEntryVal("v3", 2));
  hashtable.UpdateKV(HEntryKey("k4", 2), HEntryVal("v4", 2));
  hashtable.UpdateKV(HEntryKey("k5", 2), HEntryVal("v5", 2));

  std::vector<HEntryKey> keys = hashtable.AllKeys();
  std::vector<HEntryKey> values = hashtable.AllValues();

  displayHashTable(hashtable);

  cout << "Removing key = k3... \n";
  EXPECT_EQ(hashtable.EraseKey(HEntryKey("k3", 2)), 1);
  displayHashTable(hashtable);

  cout << "Updating key = k2 ... \n";
  EXPECT_EQ(hashtable.UpdateKV("k2", "heo"), 1);
  displayHashTable(hashtable);

  try {
    hashtable.At("k3");
  } catch (std::out_of_range &ex) {
    cout << ex.what() << endl;
    EXPECT_EQ(hashtable.At("k2"), "heo");
  }

  EXPECT_FALSE(hashtable.CheckExists("k44"));
  EXPECT_TRUE(hashtable.CheckExists("k1"));
  EXPECT_EQ(hashtable.EraseKey("k99"), 0);

  int n = 10;
  cout << "inserting " << n << " keys ...\n";
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(hashtable.UpdateKV(HEntryKey(to_string(i)), HEntryVal(to_string(i))), NEW_ADDED);
  }
  displayHashTable(hashtable);

  cout << "============== Test HashTable End ==============\n\n";
}

TEST(HashDictTest, TestHashDict) {
  HashDict dict;
  int n = 100;
  for (int i = 0; i < n; ++i) {
    dict.Update(std::to_string(i), std::to_string(i));
  }
  displayDict(dict);

  cout << "now erase some key-value... \n";
  n = 80;
  srand(time(nullptr));
  srandom(time(nullptr));
  int n_key_erased = 0;
  for (int i = 0; i < n; ++i) {
    string key = to_string(rand() % dict.Count());
    bool ans = dict.Erase(key) == ERASED;
    if (ans) ++n_key_erased;
    cout << "key -> " << key << (ans ? " erased" : " not exists") << endl;
  }
  cout << "try to erase " << n << " items, finally erased " << n_key_erased << " items\n";

  displayDict(dict);

  cout << "random access some items...\n";
  n = 60;
  srand(time(nullptr));
  srandom(time(nullptr));
  for (int i = 0; i < n; ++i) {
    string key = to_string(rand() % dict.Count());
    try {
      cout << "key = " << key << " : value = " << dict.At(key) << endl;
    } catch (std::out_of_range &ex) {
      cout << ex.what() << endl;
    }
  }
  dict.Update("tv1", "video");
  EXPECT_EQ(dict.At("tv1"), "video");
  dict.At("tv1") = HEntryVal("game");
  EXPECT_EQ(dict.At("tv1"), "game");

  cout << "============== Test HashDict End ==============\n";
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
