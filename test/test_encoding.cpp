#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <algorithm>
#include <iostream>
#include "../src/encoding.h"

TEST(EncodingTest, BasicTest) {
  auto worker = []() {
    unsigned char buf[10] = {0};
    for (uint64_t i = 1; i <= UINT32_MAX; ++i) {
      EncodeVarUnsignedInt64(i, buf);
      uint64_t dv = DecodeVarUnsignedInt64(buf);
      EXPECT_EQ(i, dv);
      if (i % 10000000 == 0) {
        printf("%lu\n", i);
      }
      memset(buf, 0, sizeof(buf));
    }
  };
  worker();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}