#include <gtest/gtest.h>
#include <iostream>
#include "../src/net/buffer.h"

using namespace std;

auto show_buffer = [](Buffer &s) {
  for (auto &c : s.ReadableAsString()) {
    if (c == '\r') {
      std::cout << "\\r";
    } else if (c == '\n') {
      std::cout << "\\n";
    } else {
      std::cout << c;
    }
  }
  cout << endl;
};

TEST(BufferTest, BasicTest) {
  Buffer buf(8);

  std::string s5 = "hell0";
  buf.Append(s5);
  std::string s6 = "wonder";
  buf.Append(s6);

  char ch7[7] = {'1', '2', '3', '4', '5', '6', '7'};

  show_buffer(buf);
  EXPECT_EQ(buf.ReadableAsString(), "hell0wonder");
  buf.Append(ch7, 7);
  EXPECT_EQ(buf.ReadableAsString(), "hell0wonder1234567");

  show_buffer(buf);

  buf.ReaderIdxForward(14);
  show_buffer(buf);
  EXPECT_EQ(buf.ReadableAsString(), "4567");

  string s7 = "qwertyu";

  buf.Append(s7);
  show_buffer(buf);
  EXPECT_EQ(buf.ReadableAsString(), "4567qwertyu");

  cout << "-------------------------------\n";
  Buffer buf2(10);
  for (int i = 0; i < 6; i++) {
    char buf[6] = {'1', '2', '3', '4', '5', '6'};
    buf2.Append(buf, 6);
    cout << "i = " << i << ": ";
    show_buffer(buf2);
  }
};

TEST(BufferTest, BasicTest2) {
  Buffer buf3(10);
  for (int i = 0; i < 5; i++) {
    char buf[5] = {'1', '2', '3', '4', '5'};
    buf3.Append(buf, 5);
    show_buffer(buf3);
    if (i == 0) {
      EXPECT_EQ(buf3.ReadableAsString(), "12345");
    } else if (i == 1) {
      EXPECT_EQ(buf3.ReadableAsString(), "4512345");
    } else if (i == 2) {
      EXPECT_EQ(buf3.ReadableAsString(), "234512345");
    } else if (i == 3) {
      EXPECT_EQ(buf3.ReadableAsString(), "51234512345");
    } else if (i == 4) {
      EXPECT_EQ(buf3.ReadableAsString(), "3451234512345");
    }
    buf3.ReaderIdxForward(3);
  }
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
