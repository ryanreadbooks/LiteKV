#include <gtest/gtest.h>
#include "../src/net/protocol.h"

TEST(ProtocolTest, SimpleTest) {
  {
    Buffer buf;
    std::string ps = "*36\r\n$4\r\n";

    buf.Append(ps);
    buf.ReaderIdxForward(1);

    std::cout << buf.ReadableAsString() << std::endl;
    size_t step;
    EXPECT_EQ(buf.ReadLong(step), 36);
    EXPECT_EQ(step, 2);
    std::cout << step << std::endl;
    std::cout << "------\n";
  }

  {
    Buffer buf;
    std::string ps = "*36\r\n$4\r\n";

    buf.Append(ps);

    std::cout << buf.ReadableAsString() << std::endl;
    size_t step;
    std::cout << buf.ReadLongFrom(1, step) << std::endl;
    std::cout << step << std::endl;
    EXPECT_EQ(buf.ReadLongFrom(1, step), 36);
    EXPECT_EQ(step, 2);
    std::cout << "------\n";
  }

  {
    std::string s = "*3\r\n$3\r\nset\r\n$2\r\nab\r\n$3\r\ndce\r\n";
    Buffer buf;
    buf.Append(s);
    CommandCache cache;
    bool err;
    TryParseFromBuffer(buf, cache, err);
    EXPECT_EQ(cache.argc, 3);
    EXPECT_EQ(cache.argv[0], "set");
    EXPECT_EQ(cache.argv[1], "ab");
    EXPECT_EQ(cache.argv[2], "dce");
    std::cout << cache;
    std::cout << "------\n";
  }
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
