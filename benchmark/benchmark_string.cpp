#include <iostream>
#include <chrono>
#include "../src/core.h"
#include "../src/mem.h"
#ifdef TCMALLOC_FOUND
#include <gperftools/malloc_extension.h>
#include <gperftools/heap-profiler.h>
#endif

using namespace std;

const static int kNum = 1000000;
const static int kStrLen = 100;
static KVContainer engine;

int main() {

#ifdef TCMALLOC_FOUND
  cout << "tcmalloc Initialize\n";
  MallocExtension::Initialize();
#endif

  /* random insert and get kNum times */
  srand(time(NULL));
  srandom(time(NULL));
  auto begin = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kNum; ++i) {
    string value(kStrLen, "0123456789"[i % 10]);
    engine.SetString(to_string(rand() % kNum), value);
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double>  duration = end - begin;
  cout << "Random insert " << kNum << " string elements elapsed: " << duration.count() << " s" <<  endl;

  int errcode = 0;
  begin = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kNum; ++i) {
    engine.Get(to_string(rand() % kNum), errcode);
  }
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;
  cout << "Current number of total elements is " << engine.NumItems() << endl;
  cout << "Random get " << kNum << " string elements elapsed: " << duration.count() << " s" << endl;
  cout << "Memory status: ";
  cout << ProcessVmSizeAsString() << endl;
#ifdef TCMALLOC_FOUND
  char buf[8192];
  MallocExtension::instance()->GetStats(buf, sizeof buf);
  printf("%s\n", buf);
  HeapProfilerDump("end");
#endif
  return 0;
}