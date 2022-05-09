#include <iostream>
#include <fstream>
#include <random>
#include "../src/net/commands.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: ./generate_pseudo_data kNumRecord\n";
    return 0;
  }
  std::string filename = "dump.aof";

  std::ofstream ofs(filename, std::ios::app);
  char *p;
  int kNumRecord = std::strtol(argv[1], &p, 10);

  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uni_dist(1, 4);
  std::uniform_int_distribution<int> uni_dist2(1, kNumRecord);
  CommandCache cache;
  int n = 0;
  for (int i = 0; i < kNumRecord; ++i) {
    cache.Clear();
     int option = uni_dist(e1);
//    int option = OBJECT_LIST;
    std::string key = std::to_string(uni_dist2(e1) % kNumRecord);
    cache.inited = true;
    if (option == OBJECT_INT) {
      cache.argc = 3;
      cache.argv = {"set", key, std::to_string(uni_dist(e1) % kNumRecord)};
    } else if (option == OBJECT_STRING) {
      cache.argc = 3;
      std::string value(10, "0123456789"[i % 10]);
      cache.argv = {"set", key, value};
    } else if (option == OBJECT_LIST) {
      /* random push elements */
      int pop = uni_dist(e1);
      int n = uni_dist(e1) % 20;
      n = std::max(n, 5);
      std::vector<std::string> vals;
      for (int j = 0; j < n; ++j) {
        /* lpush */
        vals.emplace_back(std::to_string(j));
      }
      if (pop >= 2) {
        cache.argv = {"lpush", key};
      } else {
        /* rpush */
        cache.argv = {"rpush", key};
      }
      cache.argv.insert(cache.argv.end(), vals.begin(), vals.end());
    } else if (option == OBJECT_HASH) {
      int n = uni_dist(e1) % 20;
      n = std::max(n, 5);
      std::vector<std::string> kvs;
      for (int j = 0; j < n; ++j) {
        std::string k = std::to_string(uni_dist(e1) % kNumRecord);
        std::string v(10, "0123456789abcdef"[i % 16]);
        kvs.emplace_back(k);
        kvs.emplace_back(v);
      }
      cache.argv = {"hset", key};
      cache.argv.insert(cache.argv.end(), kvs.begin(), kvs.end());
    }
    cache.argc = cache.argv.size();
    ofs << cache.ToProtocolString();
//    std::cout << cache;
    n++;
  }
  ofs.flush();
  ofs.close();
  std::cout << "Generated n = " << n << std::endl;
  return 0;
}