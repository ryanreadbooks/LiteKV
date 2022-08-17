#include <gtest/gtest.h>
#include <iostream>
#include "../src/persistence.h"

void TestAppend(AppendableFile& file) {
  for (int i = 0; i < 1024; ++i) {
    CommandCache cmd;
    cmd.argc = 3;
    cmd.argv.emplace_back("SET");
    cmd.argv.push_back(std::to_string(i));
    cmd.argv.push_back(std::to_string(i * 2));
    file.Append(cmd);
  }
  std::cout << "Appended\n";
}

std::string dpname;

TEST(AppendableTest, SimpleTest) {
  AppendableFile file(dpname, 1024);
  //  TestAppend(file);
  //  file.FlushRightNow();
  //  /* test read file */
  //  auto t = file.ReadFromScratch();
  //  for (auto &&item : t) {
  //    std::cout << "-" << item;
  //  }
  //  std::cout << t.size() << std::endl;

  file.ReadFromScratch();
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s filename", argv[0]);
    exit(0);
  }
  dpname = argv[1];

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}