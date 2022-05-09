#include "../src/config.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: ./test_config filename");
    exit(0);
  }
  std::string filename = argv[1];
  std::cout << "Loading config parameters from " << filename << std::endl;
  Config cfg(filename);

  return 0;
}