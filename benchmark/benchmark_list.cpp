#include <iostream>
#include <chrono>
#include "../src/core.h"
#include "../src/mem.h"
#ifdef TCMALLOC_FOUND
#include <gperftools/malloc_extension.h>
#include <gperftools/heap-profiler.h>
#endif

using namespace std;

const static int kNum = 100;
const static int kListLen = 100000;
const static int kStrLen = 10;
static KVContainer engine;

int main() {
#ifdef TCMALLOC_FOUND
  cout << "tcmalloc Initialize\n";
  MallocExtension::Initialize();
#endif
  /* random generate kNum and insert kListLen elements per list*/
  srand(time(NULL));
  srandom(time(NULL));
  auto begin = std::chrono::high_resolution_clock::now();
  int errcode = 0;
  for (int i = 0; i < kNum; ++i)
  {
    Key key(to_string(rand() % kNum));
    for (int j = 0; j < kListLen / 2; ++j) {
      string value(kStrLen, "0123456789"[i % 10]);
      engine.LeftPush(key, value, errcode);
    }
    for (int j = kListLen / 2; j < kListLen; ++j) {
      string value(kStrLen, "0123456789"[i % 10]);
      engine.RightPush(key, value, errcode);
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double>  duration = end - begin;
  cout << "Random push " << kListLen << " elements into " << kNum << " list, elapsed: " << duration.count() << " s" <<  endl;

  begin = std::chrono::high_resolution_clock::now();
  string key_saved;
  for (int i = 0; i < kNum; ++i) {
    Key key(to_string(rand() % kNum));
    key_saved = key.ToStdString();
    for (int j = 0; j < kListLen / 2; ++j) {
      engine.LeftPop(key, errcode);
    }
    for (int j = kListLen / 2; j < kListLen; ++j) {
      engine.RightPop(key, errcode);
    }
  }
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;
  cout << "Current number of total elements is " << engine.NumItems() << endl;
  cout << "Random pop " << kNum << " times from list, elapsed: " << duration.count() << " s" << endl;

  cout << "Lrange_100\n";
  begin = std::chrono::high_resolution_clock::now();
  engine.ListRange(key_saved, 0, 100, errcode);
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;
  cout << "Lrange_100 took " << duration.count() << " s" << endl;

  cout << "Lrange_300\n";
  begin = std::chrono::high_resolution_clock::now();
  engine.ListRange(key_saved, 0, 300, errcode);
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;
  cout << "Lrange_300 took " << duration.count() << " s" << endl;

  cout << "Lrange_500\n";
  begin = std::chrono::high_resolution_clock::now();
  engine.ListRange(key_saved, 0, 500, errcode);
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;
  cout << "Lrange_500 took " << duration.count() << " s" << endl;

  cout << "Lrange_600\n";
  begin = std::chrono::high_resolution_clock::now();
  engine.ListRange(key_saved, 0, 600, errcode);
  end = std::chrono::high_resolution_clock::now();
  duration = end - begin;
  cout << "Lrange_600 took " << duration.count() << " s" << endl;

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