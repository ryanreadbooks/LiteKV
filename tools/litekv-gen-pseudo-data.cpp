#include <iostream>
#include <fstream>
#include <random>
#ifdef TCMALLOC_FOUND
#include <gperftools/malloc_extension.h>
#endif

#include "../src/net/commands.h"

int main(int argc, char **argv) {
#ifdef TCMALLOC_FOUND
  MallocExtension::Initialize();
#endif

  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " kNumRecord dumpfile\n";
    return 0;
  }
  std::string filename = argv[2];

  std::ofstream ofs(filename, std::ios::app | std::ios::binary);
  char *p;
  long kNumRecord = std::strtol(argv[1], &p, 10);

  std::random_device r;
  std::mt19937_64 rand_engine(r());
  std::uniform_int_distribution<int> uni_dist(1, 4);
  std::uniform_int_distribution<int> uni_dist2(1, kNumRecord);
  std::uniform_int_distribution<int> uni_dist3(3, 15);

  auto GenRandomStr = [&](int n) -> std::string {
    return std::string(n, "0123456789abcdefghijklmnopqrstuvwxyz"[uni_dist2(rand_engine) % 36]);
  };

  auto GenRandomStr2 = [&](int n) -> std::string {
    std::string ans;
    for (int i = 0; i < n; ++i) {
      ans += GenRandomStr(1);
    }
    return ans;
  };

  CommandCache cache;
  int n = 0;
  for (int i = 0; i < kNumRecord; ++i) {
    cache.Clear();
    int option = uni_dist(rand_engine);
//    int option = OBJECT_LIST;
    std::string key = std::to_string(uni_dist2(rand_engine) % kNumRecord);
    cache.inited = true;
    if (option == OBJECT_INT) {
      cache.argc = 3;
      cache.argv = {"set", key, std::to_string(uni_dist(rand_engine) % kNumRecord)};
    } else if (option == OBJECT_STRING) {
      cache.argc = 3;
      std::string value = GenRandomStr2(uni_dist3(rand_engine));
      cache.argv = {"set", key, value};
    } else if (option == OBJECT_LIST) {
      /* random push elements */
      int pop = uni_dist(rand_engine);
      int n = uni_dist(rand_engine) % 20;
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
      int n = uni_dist(rand_engine) % 20;
      n = std::max(n, 5);
      std::vector<std::string> kvs;
      for (int j = 0; j < n; ++j) {
        std::string k = std::to_string(uni_dist(rand_engine) % kNumRecord) + GenRandomStr(uni_dist(rand_engine));
        std::string v = GenRandomStr2(uni_dist3(rand_engine));
        kvs.emplace_back(k);
        kvs.emplace_back(v);
      }
      cache.argv = {"hset", key};
      cache.argv.insert(cache.argv.end(), kvs.begin(), kvs.end());
    }
    cache.argc = cache.argv.size();
    ofs << cache.ToProtocolString();
    n++;
  }
  ofs.flush();
  ofs.close();
  std::cout << "Generated n = " << n << std::endl;
  return 0;
}