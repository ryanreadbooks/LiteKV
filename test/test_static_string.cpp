#include <gtest/gtest.h>
#include <array>
#include <iostream>
#include "../src/str.h"

TEST(StaticStringTest, BasicTest) {
  StaticString s("hello world");
  std::string s2 = "wonder";
  StaticString fs2(s2);
  EXPECT_EQ(s.Length(), 11);
  EXPECT_STREQ(s.Data(), "hello world");
  EXPECT_EQ(fs2.Length(), 6);
  std::cout << fs2 << std::endl;
  EXPECT_STREQ(fs2.Data(), "wonder");
  StaticString fs3("hello world");
  EXPECT_TRUE(fs3 == s);

  StaticString fs4 = fs2;
  EXPECT_EQ(fs4.ToStdString(), "wonder");

  fs4 = fs3;
  EXPECT_EQ(fs4.ToStdString(), "hello world");

  std::cout << "sizeof(FixedSizeString)=" << sizeof(StaticString) << std::endl;

  std::cout << "sizeof(array)=" << sizeof(std::array<int, 16>) << std::endl;  // 4 * 16 = 64
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
