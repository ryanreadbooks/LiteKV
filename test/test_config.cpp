#include <gtest/gtest.h>
#include "../src/config.h"

std::string filename;

TEST(ConfigTest, BasicTest) {
  std::cout << "Loading config parameters from " << filename << std::endl;
  Config cfg(filename);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s filename", argv[0]);
    exit(0);
  }
  filename = argv[1];
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}