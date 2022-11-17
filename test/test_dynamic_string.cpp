#include <gtest/gtest.h>
#include <iostream>
#include "../src/str.h"

using namespace std;

TEST(DynamicStringTest, BasicTest) {
  DynamicString str1("asd", 3);
  string stl_str = "C++JavaPython";
  DynamicString str2(stl_str);
  EXPECT_EQ(str1, "asd");
  EXPECT_EQ(str2, "C++JavaPython");

  str1.Append(str2);
  EXPECT_EQ(str1, "asdC++JavaPython");
  EXPECT_EQ(str1.Length(), 16);
  EXPECT_EQ(str1.Allocated(), 25);
  EXPECT_EQ(str2.Length(), 13);
  EXPECT_EQ(str2.Allocated(), 14);
  str1.Append("hihi", 4);
  EXPECT_EQ(str1.Length(), 20);
  EXPECT_EQ(str1.Allocated(), 25);
  for (size_t i = 0; i <= str1.Length(); i++) {
    if (str1[i] == '\0') {
      cout << "\nstr1 ends at " << i << endl;
    } else {
      cout << str1[i] << "|";
    }
  }

  DynamicString str3("ok\0okok", 7);
  EXPECT_STREQ(str3.ToStdString().c_str(), "ok\0okok");
  for (size_t i = 0; i < str3.Length(); i++) {
    if (str3[i] == '\0') {
      cout << "~";
    } else {
      cout << str3[i];
    }
  }
  cout << endl;

  DynamicString str4("123456", 6);
  for (int i = -1; i >= -6; i--) {
    cout << str4[i] << "|";
  }
  cout << endl;

  // compare test
  DynamicString c1("hello", 5);
  DynamicString c2("hello", 5);
  EXPECT_TRUE(c1 == c2);
  DynamicString c3("ehllo", 5);
  EXPECT_TRUE(c1 != c3);
  c3.Clear();
  EXPECT_TRUE(c3.Empty());

  // test reset string value
  c2.Reset("are you ok? I am really fine!!!");
  cout << "c2 = " << c2 << endl;
  EXPECT_EQ(c2, "are you ok? I am really fine!!!");
  EXPECT_EQ(c2.Length(), 31);
  EXPECT_EQ(c2.Allocated(), 47);
  c2.Reset("okok");
  EXPECT_EQ(c2, "okok");
  EXPECT_EQ(c2.Length(), 4);
  EXPECT_EQ(c2.Allocated(), 47);
  c2.Shrink();
  EXPECT_EQ(c2, "okok");
  EXPECT_EQ(c2.Length(), 4);
  EXPECT_EQ(c2.Allocated(), 5);
  std::string cst2("monday");
  c2.Append(cst2);
  EXPECT_EQ(c2, "okokmonday");
  EXPECT_EQ(c2.Length(), 10);
  EXPECT_EQ(c2.Allocated(), 16);

  DynamicString cstr3;
  cstr3.Clear();
  cstr3.Reset("okok");
  EXPECT_EQ(cstr3, "okok");
  EXPECT_EQ(cstr3.Length(), 4);
  EXPECT_EQ(cstr3.Allocated(), 5);

  DynamicString cstr4;
  cstr4.Append("abc");
  EXPECT_EQ(cstr4.Length(), 3);
  EXPECT_EQ(cstr4.Allocated(), 4);

  cstr4.Append("abcefg");
  EXPECT_EQ(cstr4.Length(), 9);
  EXPECT_EQ(cstr4.Allocated(), 14);

  auto func = []() -> DynamicString { return DynamicString(); };

  DynamicString cstr5;
  DynamicString cstr6 = func();
  EXPECT_TRUE(cstr5.Empty());
  EXPECT_TRUE(cstr6.Empty());

  // test reset
  DynamicString ds1("10");
  ds1.Clear();
  ds1.Reset("changed");
  EXPECT_EQ(ds1, "changed");

  // test empty string and null string
  // no memory allocated
  DynamicString empty;
  EXPECT_TRUE(empty.Length() == 0 && empty.Allocated() == 0);

  // memory allocated even for empty str
  DynamicString empty2("", 0);
  EXPECT_TRUE(empty2.Length() == 0 && empty2.Allocated() == 1);

  // memory allocated even for empty str
  DynamicString empty3("");
  EXPECT_TRUE(empty3.Length() == 0 && empty3.Allocated() == 1);
}

TEST(DynamicStringTest, ComparatorTest) {
  DynamicString s1("hello");
  DynamicString s2("hello world");
  EXPECT_TRUE(s1 < s2);

  DynamicString s3("z");
  EXPECT_TRUE(s1 < s3);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
