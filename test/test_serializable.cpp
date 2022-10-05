#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include "../src/net/net.h"
#include "../src/core.h"


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

  StaticString s2;
  bin.clear();
  s2.Serialize(bin);
  std::cout << "bin.size() = " << bin.size() << '\n';
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
  unlink("tmp");
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
  unlink("tmp_hs");
}

TEST(SerializableTest, KVContainerTest) {
  KVContainer kv;
  int errcode;
   kv.SetInt("int1", 0x01234567abcdef98);
   kv.SetInt("int2", -20201003);
   kv.SetInt("int3", 0);
   kv.SetInt("int4", 100);

   kv.SetString("str1", "hello");
   kv.SetString("str2", "world");
   kv.SetString("str3", "abcdef");
   kv.SetString("str4", "");

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

  // save
  LiteKVSave("dump.lkvdb", bin);
}

void LoadCallback(size_t& total, size_t& progress) {
  std::cout << "Total: " << total << ", Progress -> " << progress << '\n';
}

TEST(SerializableTest, LKVDbLoadTest) {
  std::string src = "dump.lkvdb";
  KVContainer holder;
  EventLoop loop;
  LiteKVLoad(src, &holder, &loop, LoadCallback);
  auto vec = holder.Overview();
  EXPECT_STREQ(vec[0].ToStdString().c_str(), "Number of int:");
  EXPECT_STREQ(vec[1].ToStdString().c_str(), "4");
  EXPECT_STREQ(vec[2].ToStdString().c_str(), "Number of string:");
  EXPECT_STREQ(vec[3].ToStdString().c_str(), "4");
  EXPECT_STREQ(vec[4].ToStdString().c_str(), "Number of list:");
  EXPECT_STREQ(vec[5].ToStdString().c_str(), "2");
  EXPECT_STREQ(vec[6].ToStdString().c_str(), "Number of elements in list:");
  EXPECT_STREQ(vec[7].ToStdString().c_str(), "7");
  EXPECT_STREQ(vec[8].ToStdString().c_str(), "Number of hash:");
  EXPECT_STREQ(vec[9].ToStdString().c_str(), "2");
  EXPECT_STREQ(vec[10].ToStdString().c_str(), "Number of elements in hash:");
  EXPECT_STREQ(vec[11].ToStdString().c_str(), "5");
  EXPECT_STREQ(vec[12].ToStdString().c_str(), "Number of set:");
  EXPECT_STREQ(vec[13].ToStdString().c_str(), "3");
  EXPECT_STREQ(vec[14].ToStdString().c_str(), "Number of elements in set:");
  EXPECT_STREQ(vec[15].ToStdString().c_str(), "8");

  unlink("dump.lkvdb");
}

TEST(SerializableTest, EmptyKeyTest) {
  KVContainer kv;
  int errcode;
  kv.SetString("hello", "world");
  kv.SetInt("int1", 100362);
  kv.SetString("", "");
  kv.SetString("wow", "");
  std::vector<char> bin;
  kv.Snapshot(bin);

  // save
  LiteKVSave("dump.lkvdb", bin);

  EventLoop loop;
  LiteKVLoad("dump.lkvdb", &kv, &loop, LoadCallback);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}