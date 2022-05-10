#include <iostream>
#include "../src/persistence.h"

void TestAppend(AppendableFile &file) {
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

int main(int argc, char** argv) {
  std::string dpname = argv[1];
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

  return 0;
}