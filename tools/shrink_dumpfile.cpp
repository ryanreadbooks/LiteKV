#include <iostream>

#ifdef TCMALLOC_FOUND

#include <gperftools/malloc_extension.h>

#endif

#include "../src/persistence.h"

int main(int argc, char **argv) {
#ifdef TCMALLOC_FOUND
  MallocExtension::Initialize();
#endif
  if (argc != 3) {
    std::cout << "Usage: ./shrink_dumpfile sourcefile destinationfile";
    exit(EXIT_SUCCESS);
  }
  std::string source = argv[1];
  std::string destination = argv[2];
  std::cout << "shrink dumpfile from " << source << " to " << destination << "...\n";

  try {
    AppendableFile file(destination, 4096, true);
    auto begin = GetCurrentMs();
    file.RemoveRedundancy(source);
    auto spent = GetCurrentMs() - begin;

    std::cout << "Process done took " << spent << " ms\n";

    std::cout << "Done!\n";
  } catch (const std::exception& ex) {
    std::cerr << "Exception occurred!! " << ex.what() << std::endl;
    std::cerr << "Can not recognize source dumpfile!! Can not finish procedure.\n";
  }

  return 0;
}