#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <random>
#include "../src/core.h"
#include "../src/lkvdb.h"

std::mt19937_64 sRandEngine(std::time(nullptr));

int rand_int(int64_t from, int64_t to) {
  return std::uniform_int_distribution<int64_t>(from, to)(sRandEngine);
}

TEST(LKVDBTEST, TestLKVDB) {
  static std::string dumpfile = "dump.lkvdb";
  // random generate all kinds of key-value pair
  KVContainer original;
  // randomly insert key-value pairs
  // 1. integer
  std::unordered_map<std::string, int64_t> integers;
  int n_integer = rand_int(1000, 10000);
  for (int i = 0; i < n_integer; ++i) {
    int64_t val = rand_int(INT64_MIN, INT64_MAX);
    std::string key = "int-" + std::to_string(i);
    integers[key] = val;
    original.SetInt(key, val);
  }

  // 2. string
  static std::string ascii;
  for (int i = 33; i <= 126; ++i) {
    ascii.push_back(i);
  }
  std::cout << "ASCII CHAR CANDIDATES = " << ascii << std::endl;
  std::unordered_map<std::string, std::string> strings;
  int n_string = rand_int(1000, 10000);
  for (int i = 0; i < n_string; ++i) {
    std::string key = "str-" + std::to_string(i);
    std::string val;
    int size_val = rand_int(1, 1000);
    for (int j = 0; j < size_val; ++j) {
      val.push_back(ascii[rand_int(0, ascii.size() - 1)]);
    }
    strings[key] = val;
    original.SetString(key, val);
  }
  // put an empty key in it
  strings[std::string()] = std::string();
  original.SetString(std::string(),std::string());

  int errcode;
  // 3. list
  std::unordered_map<std::string, std::vector<std::string>> lists;
  int n_list = rand_int(1000, 10000);
  for (int i = 0; i < n_list; ++i) {
    std::string key = "list-" + std::to_string(i);
    // every list has n_elem elements
    int n_elem = rand_int(1, 100);
    for (int j = 0 ; j < n_elem; ++j) {
      int size_val = rand_int(1, 10);
      std::string val;
      for (int k = 0; k < size_val; ++k) {
        val.push_back(ascii[rand_int(0, ascii.size() - 1)]);
      }
      original.RightPush(key, val, errcode);
      lists[key].push_back(val);
    }
  }

  // 4. hash
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hashes;
  int n_hash = rand_int(1000, 10000);
  for (int i = 0; i < n_hash; ++i) {
    std::string key = "hash-" + std::to_string(i);
    // every hash has n_elem elements;
    int n_elem = rand_int(1, 100);
    for (int j = 0; j < n_elem; ++j) {
      int str_val = rand_int(1, 100);
      std::string hash_key, hash_val;
      for (int k = 0; k < str_val; ++k) {
        hash_key.push_back(ascii[rand_int(0, ascii.size() - 1)]);
        hash_val.push_back(ascii[rand_int(0, ascii.size() - 1)]);
      }
      original.HashUpdateKV(key, hash_key, hash_val, errcode);
      hashes[key][hash_key] = hash_val;
    }
  }
  // 5. set
  std::unordered_map<std::string, std::unordered_set<std::string>> sets;
  int n_set = rand_int(1000, 10000);
  for (int i = 0; i < n_set; ++i) {
    std::string key = "set-" + std::to_string(i);
    int n_elem = rand_int(1, 100);
    for (int j = 0; j < n_elem; ++j) {
      std::string val;
      int val_len = rand_int(1, 20);
      for (int k = 0; k < val_len; ++k) {
        val.push_back(ascii[rand_int(0, ascii.size() - 1)]);
      }
      original.SetAddItem(key, val, errcode);
      sets[key].insert(val);
    }
  }

  // and then store then in memory
  std::vector<char> bin;
  bin.reserve(2048);
  original.Snapshot(bin);
  LiteKVSave(dumpfile, bin);
//   then reload the saved dumpfile to restore data,

  KVContainer restored;
  EventLoop loop;
  LiteKVLoad(dumpfile, &restored, &loop, [] (size_t& total, size_t progress) {});

  // and compare it to the original one
  EXPECT_EQ(restored.NumItems(), original.NumItems());
  // WE expect every key-value pairs are identical
  std::vector<DynamicString> original_overview = original.Overview();
  std::vector<DynamicString> restore_overview = restored.Overview();
  EXPECT_EQ(original_overview.size(), restore_overview.size());
  for (size_t i = 0; i < original_overview.size(); ++i) {
    EXPECT_STREQ(original_overview[i].ToStdString().c_str(), restore_overview[i].ToStdString().c_str());
  }
  // check integer
  for (auto& item : integers) {
    ValueObjectPtr obj = restored.Get(item.first, errcode);
    EXPECT_EQ(errcode, kOkCode);
    EXPECT_EQ(item.second, obj->ToInt64());
  }

  // check string
  for (auto& item : strings) {
    ValueObjectPtr obj = restored.Get(item.first, errcode);
    EXPECT_EQ(errcode, kOkCode);
    EXPECT_STREQ(item.second.c_str(), obj->ToStdString().c_str());
  }

  // check list
  for (auto& item : lists) {
    std::vector<std::string> items = restored.ListRangeAsStdString(item.first,0,-1, errcode);
    EXPECT_EQ(errcode, kOkCode);
    std::vector<std::string>& target = item.second;
    EXPECT_EQ(items.size(), target.size());
    for (size_t i = 0; i < items.size(); ++i) {
      EXPECT_STREQ(items[i].c_str(), target[i].c_str());
    }
  }

  // check hash
  for (auto& item : hashes) {
    const std::string& key = item.first;
    std::unordered_map<std::string, std::string>& pairs = item.second;
    // check every key-value pairs in pairs
    std::vector<DynamicString> entries = restored.HashGetAllEntries(key, errcode);
    EXPECT_EQ(errcode, kOkCode);
    std::vector<DynamicString> all_fields = restored.HashGetAllFields(key, errcode);
    EXPECT_EQ(errcode, kOkCode);
    std::vector<DynamicString> all_values = restored.HashGetAllValues(key, errcode);
    EXPECT_EQ(errcode, kOkCode);
    EXPECT_EQ(pairs.size(), entries.size() / 2);
    EXPECT_EQ(all_fields.size(), all_values.size());
    EXPECT_EQ(all_fields.size(), pairs.size());
    for (auto& kv : pairs) {
      DynamicString v = restored.HashGetValue(key, kv.first, errcode);
      EXPECT_EQ(errcode, kOkCode);
      EXPECT_STREQ(kv.second.c_str(), v.ToStdString().c_str());
    }
  }

  // check set
  for (auto& item : sets) {
    const std::string& key = item.first;
    std::unordered_set<std::string>& elements = item.second;
    // check every elements
    size_t mem_cnt = restored.SetGetMemberCount(key, errcode);
    EXPECT_EQ(errcode, kOkCode);
    EXPECT_EQ(mem_cnt, elements.size());
    std::vector<DynamicString> rev = restored.SetGetMembers(key, errcode);
    EXPECT_EQ(errcode, kOkCode);
    size_t cnt = 0;
    // every elements in rev should be found in elements
    for (auto& ele : rev) {
      if (elements.count(ele.ToStdString())) {
        cnt++;
      }
    }
    EXPECT_EQ(cnt, elements.size());
    EXPECT_EQ(cnt, rev.size());
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}