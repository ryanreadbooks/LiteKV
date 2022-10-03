#include <gtest/gtest.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include "../src/core.h"
#include "../src/dlist.h"
#include "../src/encoding.h"
#include "../src/hashdict.h"
#include "../src/hashset.h"
#include "../src/str.h"

unsigned char buf[12] = {0x0b, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64};

TEST(SerializableTest, StaticStringTest) {
  StaticString s("hello world");

  std::vector<char> bin;
  s.Serialize(bin);
  for (size_t i = 0; i < bin.size(); ++i) {
    printf("%02x ", (unsigned char)bin[i]);
    EXPECT_EQ(bin[i], buf[i]);
  }
  printf("\n");
}

TEST(SerializableTest, DynamicStringTest) {
  DynamicString s("hello world");
  std::vector<char> bin;
  s.Serialize(bin);
  for (size_t i = 0; i < bin.size(); ++i) {
    printf("%02x ", (unsigned char)bin[i]);
    EXPECT_EQ(bin[i], buf[i]);
  }
  printf("\n");
}

TEST(SerializableTest, DListTest) {
  DList list;
  list.PushRight("ab");
  list.PushRight("cd");
  list.PushRight("ef");
  list.PushRight("gh");
  list.PushRight("hello");
  list.PushRight("world");

  std::vector<char> bin;
  list.Serialize(bin);
  printf("size = %lu\n", bin.size());
  for (size_t i = 0; i < bin.size(); ++i) {
    printf("%02x ", (unsigned char)bin[i]);
  }
  printf("\n");
}

TEST(SerializableTest, HashDictTest) {
  HashDict hd(0.1);
  hd.Update("name", "john");
  hd.Update("age", "25");
  hd.Update("id", "0011");
  hd.Update("word", "wonder");
  hd.Update("ide", "vscode");

  hd.Update("longstr",
            "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefgh"
            "ijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");  // 6

  std::vector<char> bin;
  hd.Serialize(bin);
  printf("size = %lu\n", bin.size());
  for (size_t i = 0; i < bin.size(); ++i) {
    printf("%02x ", (unsigned char)bin[i]);
  }
  printf("\n");
  std::ofstream ofs("tmp", std::ios::out | std::ios::trunc);
  ofs.write(bin.data(), bin.size());
  ofs.close();
}

TEST(SerializableTest, HashSetTest) {
  HashSet hs(0.1);
  hs.Insert("1");
  hs.Insert("2");
  hs.Insert("3");
  hs.Insert("4");
  hs.Insert("5");
  hs.Insert("6");
  hs.Insert("7");
  hs.Insert("8");
  hs.Insert("9");
  hs.Insert("10");
  hs.Insert("11");
  hs.Insert("12");
  hs.Insert("13");
  hs.Insert("14");
  hs.Insert("15");
  hs.Insert("16");
  hs.Insert("17");
  hs.Insert("18");

  hs.Insert(
      "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefgh"
      "ijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");  // 6

  std::vector<char> bin;
  hs.Serialize(bin);
  printf("size = %lu\n", bin.size());
  for (size_t i = 0; i < bin.size(); ++i) {
    printf("%02x ", (unsigned char)bin[i]);
  }
  printf("\n");
  std::ofstream ofs("tmp_hs", std::ios::out | std::ios::trunc);
  ofs.write(bin.data(), bin.size());
  ofs.close();
}

TEST(SerializableTest, KVContainerTest) {
  KVContainer kv;
  int errcode;
   kv.SetInt("int1", 0x01234567abcdef98);
   kv.SetInt("int2", -20201003);

   kv.SetString("str1", "hello");
   kv.SetString("str2", "world");
   kv.SetString("str3", "abcdef");

   kv.RightPush("list1", "789", errcode);
   kv.RightPush("list1", "ful", errcode);
   kv.RightPush("list1", "vscode", errcode);
   kv.RightPush("list1", "wonder", errcode);
   kv.LeftPush("list2", "ide", errcode);
   kv.LeftPush("list2", "c++", errcode);
   kv.LeftPush("list2", "java", errcode);

  EXPECT_TRUE(kv.HashUpdateKV("dict1", "name", "john", errcode));
  EXPECT_TRUE(kv.HashUpdateKV("dict1", "age", "35", errcode));
  EXPECT_TRUE(kv.HashUpdateKV("dict1", "place", "Whomton", errcode));
  EXPECT_TRUE(kv.HashUpdateKV("dict2", "day", "monday", errcode));
  EXPECT_TRUE(kv.HashUpdateKV("dict2", "my-id", "0d9854wasd", errcode));

   kv.SetAddItem("set1", "q", errcode);
   kv.SetAddItem("set1", "w", errcode);
   kv.SetAddItem("set1", "e", errcode);
   kv.SetAddItem("set1", "r", errcode);
   kv.SetAddItem("set2", "left", errcode);
   kv.SetAddItem("set2", "right", errcode);
   kv.SetAddItem("set2", "up", errcode);
   kv.SetAddItem("set3", "e", errcode);

  std::vector<char> bin;
  kv.Snapshot(bin);

  std::ofstream ofs("kvtmp", std::ios::out | std::ios::trunc);
  ofs.write(bin.data(), bin.size());
  ofs.close();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}