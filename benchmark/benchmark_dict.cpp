#include <iostream>
#include <chrono>
#include "../src/core.h"
#include "../src/profile.h"

#ifdef TCMALLOC_FOUND

#include <gperftools/malloc_extension.h>
#include <gperftools/heap-profiler.h>

#endif

using namespace std;

const static int kNum = 100000;
const static int kEntry = 100;
static KVContainer engine;

int main() {
#ifdef TCMALLOC_FOUND
  cout << "tcmalloc Initialize\n";
  MallocExtension::Initialize();
#endif
  /* random insert and get kNum times */
  cout << "Random putting " << kEntry << " items into every " << kNum << " hashtable..." << endl;
  srand(time(NULL));
  srandom(time(NULL));
  int errcode;
  auto begin = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kNum; ++i) {
    for (int j = 0; j < kEntry; ++j) {
      string field = to_string(rand());
      string val = to_string(rand());
      engine.HashSetKV(to_string(i), field, val, errcode);
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - begin;
  cout << "Random putting " << kNum << " hash elements ("
       << kEntry << " entries each) elapsed: " << duration.count() << " s" << endl;

  cout << "Random query " << kNum * kEntry << " times...\n";
  begin = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kNum * kEntry; ++i) {
    string key = to_string(rand() % kNum);
    string field = to_string(rand());
    engine.HashGetValue(key, field, errcode);
  }
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;

  cout << "Current number of total elements is " << engine.NumItems() << endl;
  cout << "Random get " << kNum * kEntry << " elements elapsed: " << duration.count() << " s" << endl;

  cout << "Container overview: \n";
  cout << engine.Overview() << endl;

  cout << "Memory status: ";
  cout << ProcessVmSize() << endl;
#ifdef TCMALLOC_FOUND
  char buf[8192];
  MallocExtension::instance()->GetStats(buf, sizeof buf);
  printf("%s\n", buf);
  HeapProfilerDump("end");
#endif
  return 0;
}